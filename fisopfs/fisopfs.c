#define FUSE_USE_VERSION 30

#include <stdbool.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_NAME_LEN 120
#define MAX_CONTENT_LEN 1024

typedef struct fisopfs_file {
	char name[MAX_NAME_LEN];  // agregar ESTATICO
	char content[MAX_CONTENT_LEN];
	struct stat st;
	struct fisopfs_file *next;
} fisopfs_file_t;

typedef struct fisopfs_sb {
	char *disk_path;
	fisopfs_file_t *root;
	size_t size;
} fisopfs_sb_t;

// Función auxiliar para sb
fisopfs_sb_t *
get_sb()
{
	struct fuse_context *c = fuse_get_context();
	return c->private_data;
}

fisopfs_file_t **
get_sb_root()
{
	fisopfs_sb_t *sb = get_sb();
	return &sb->root;
}

void
set_sb_root(fisopfs_file_t *r)
{
	fisopfs_sb_t *sb = get_sb();
	sb->root = r;
}

// Función auxiliar para buscar un archivo
static fisopfs_file_t *
fisopfs_find(const char *name)
{
	fisopfs_file_t *file = (*get_sb_root());

	while (file) {
		if (strcmp(file->name, name) == 0)
			return file;
		file = file->next;
	}
	return NULL;
}

static int
fisopfs_getattr(const char *path, struct stat *st)
{
	fisopfs_file_t *file;

	if (strcmp(path, "/") == 0) {
		st->st_uid = getuid();
		st->st_gid = getgid();
		st->st_mode = __S_IFDIR | 0755;
		st->st_nlink = 2;
	} else {
		file = fisopfs_find(path + 1);  // +1 para saltar el '/'
		if (!file)
			return -ENOENT;
		memcpy(st, &file->st, sizeof(struct stat));
	}

	return 0;
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	fisopfs_file_t *file = *get_sb_root();
	while (file) {
		if (strcmp(path, "/") == 0) {
			// Si estamos en el directorio raíz, listamos todo excepto lo que está en subdirectorios
			if (strchr(file->name, '/') == NULL) {
				filler(buffer, file->name, NULL, 0);
			}
		} else {
			// Estamos en un subdirectorio, listamos solo los archivos de este subdirectorio
			const char *subdir_path =
			        path + 1;  // Omitir el '/' inicial
			size_t subdir_len = strlen(subdir_path);
			if (strncmp(file->name, subdir_path, subdir_len) == 0 &&
			    file->name[subdir_len] == '/' &&
			    strchr(file->name + subdir_len + 1, '/') == NULL) {
				filler(buffer, file->name + subdir_len + 1, NULL, 0);
			}
		}
		file = file->next;
	}

	return 0;
}

static int
fisopfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	if (fisopfs_find(path + 1))
		return -EEXIST;  // Verifica si el archivo ya existe

	// Crea un nuevo nodo para el archivo
	fisopfs_file_t *new_file = malloc(sizeof(fisopfs_file_t));
	if (!new_file)
		return -ENOMEM;  // Verifica si la asignación de memoria fue exitosa

	if (!new_file->name) {
		free(new_file);  // Libera la memoria del nodo si la asignación del nombre falla
		return -ENOMEM;
	}
	strcpy(new_file->name, path + 1);  // Copia el nombre del archivo

	// Inicializa los atributos del archivo
	new_file->st.st_uid = getuid();  // ID del usuario propietario
	new_file->st.st_gid = getgid();  // ID del grupo propietario
	new_file->st.st_mode = mode;     // Modo (permisos) del archivo
	new_file->st.st_nlink = 1;       // Número de enlaces
	new_file->st.st_size = 0;  // Tamaño del archivo (0 para nuevo archivo)

	time_t current_time;
	time(&current_time);
	new_file->st.st_ctime = current_time;

	// Inserta el nuevo archivo en la lista enlazada
	fisopfs_file_t **aux = get_sb_root();
	new_file->next =
	        *aux;  // esto siempre es NULL, ya que es la creación inicial
	*aux = new_file;

	return 0;  // Retorna éxito
}

static int
fisopfs_write(const char *path,
              const char *buf,
              size_t size,
              off_t offset,
              struct fuse_file_info *fi)
{
	fisopfs_file_t *file = fisopfs_find(path + 1);
	if (!file)
		return -ENOENT;

	if (offset + size > MAX_CONTENT_LEN) {
		return -ENOMEM;
	}

	if (offset + size >= file->st.st_size) {
		file->st.st_size = offset + size;
	}

	// Escribe los datos
	memcpy(file->content + offset, buf, size);

	// Actualiza la hora de modificación y cambio
	time_t current_time;
	time(&current_time);
	file->st.st_mtime = current_time;
	file->st.st_ctime = current_time;

	return size;
}

static int
fisopfs_utimens(const char *path, const struct timespec ts[2])
{
	fisopfs_file_t *file = fisopfs_find(path + 1);  // Busca el archivo
	if (!file)
		return -ENOENT;  // Si el archivo no existe, retorna un error

	// Actualiza las fechas de acceso y modificación
	file->st.st_atime = ts[0].tv_sec;
	file->st.st_mtime = ts[1].tv_sec;

	return 0;  // Retorna éxito
}

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	fisopfs_file_t *file =
	        fisopfs_find(path + 1);  // Buscar el archivo en la lista
	if (!file)
		return -ENOENT;

	// Comprobar si el offset es válido
	if (offset >= file->st.st_size)
		return 0;

	// Ajustar el tamaño de la lectura si es necesario
	if (offset + size > file->st.st_size)
		size = file->st.st_size - offset;

	// Leer los datos del archivo
	memcpy(buffer, file->content + offset, size);

	// Actualiza la hora de último acceso
	time_t current_time;
	time(&current_time);
	file->st.st_atime = current_time;

	return size;
}

static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	if (fisopfs_find(path + 1))
		return -EEXIST;

	fisopfs_file_t *new_dir = malloc(sizeof(fisopfs_file_t));
	if (!new_dir)
		return -ENOMEM;

	strcpy(new_dir->name, path + 1);

	new_dir->st.st_uid = getuid();
	new_dir->st.st_gid = getgid();
	new_dir->st.st_mode = __S_IFDIR | mode;
	new_dir->st.st_nlink = 2;

	// Establece las fechas de acceso y modificación al tiempo actual
	time_t current_time;
	time(&current_time);
	new_dir->st.st_atime = current_time;
	new_dir->st.st_mtime = current_time;
	new_dir->st.st_ctime = current_time;

	fisopfs_file_t **aux = get_sb_root();
	new_dir->next =
	        *aux;  // esto siempre es NULL, ya que es la creación inicial
	*aux = new_dir;

	return 0;
}

static int
fisopfs_rmdir(const char *path)
{
	fisopfs_file_t **ptr = get_sb_root();

	while (*ptr) {
		if (strcmp((*ptr)->name, path + 1) == 0 &&
		    S_ISDIR((*ptr)->st.st_mode)) {
			// Comprobar si el directorio está vacío
			fisopfs_file_t *check = (*get_sb_root());
			while (check) {
				if (strncmp(check->name,
				            path + 1,
				            strlen(path + 1)) == 0 &&
				    strcmp(check->name, path + 1) != 0) {
					return -ENOTEMPTY;  // Directorio no está vacío
				}
				check = check->next;
			}
			// Eliminar el directorio
			fisopfs_file_t *to_delete = *ptr;
			*ptr = to_delete->next;

			free(to_delete);
			return 0;
		}
		ptr = &(*ptr)->next;
	}
	return -ENOENT;
}

static int
fisopfs_unlink(const char *path)
{
	fisopfs_file_t **ptr = get_sb_root();
	while (*ptr) {
		if (strcmp((*ptr)->name, path + 1) == 0) {
			fisopfs_file_t *to_delete = *ptr;
			*ptr = to_delete->next;
			free(to_delete);
			return 0;
		}
		ptr = &(*ptr)->next;
	}
	return -ENOENT;
}

// The return value will passed in the private_data field of struct fuse_context to all file operations.
static void *
fisopfs_init(struct fuse_conn_info *conn)
{
	fisopfs_sb_t *sb = get_sb();

	// abro el archivo solicitado para cargar el FS
	FILE *f = fopen(sb->disk_path, "rb");
	if (f == NULL) {
		return sb;
	}

	// copio los nodos que estan guardados en el archivo
	fisopfs_file_t *iter = sb->root;
	fisopfs_file_t aux;
	while (fread(&aux, sizeof(fisopfs_file_t), 1, f)) {
		fisopfs_file_t *new_file = malloc(sizeof(fisopfs_file_t));
		if (!new_file) {
			perror("error al pedir memoria");
			exit(1);
		}

		// copio los valores leidos
		*new_file = aux;

		// si el root es NULL, significa que es mi primer elemento
		if (sb->root == NULL) {
			sb->root = new_file;
			iter = new_file;
			iter->next = NULL;
			continue;
		}

		new_file->next = NULL;
		iter->next = new_file;
		iter = new_file;
	}

	fclose(f);

	return sb;
}


static void
persist_fs(fisopfs_sb_t *sb)
{
	// abro el archivo para escribir
	FILE *f = fopen(sb->disk_path, "wb");
	if (f == NULL) {
		perror("error al volcar los datos en el disco");
		exit(1);
	}

	// escribo nodo por nodo en el archivo
	fisopfs_file_t *aux = sb->root;
	while (aux) {
		if (!fwrite(aux, sizeof(fisopfs_file_t), 1, f)) {
			perror("error al escribir el sb");
			exit(1);
		}


		aux = aux->next;
	}

	fclose(f);
}

static int
fisopfs_flush(const char *path, struct fuse_file_info *fi)
{
	fisopfs_sb_t *sb = get_sb();
	persist_fs(sb);

	return 0;
}


static void
fisopfs_destroy(void *private_data)
{
	fisopfs_sb_t *sb = (fisopfs_sb_t *) private_data;
	persist_fs(sb);
}

static int
fisopfs_truncate(const char *path, off_t size)
{
	fisopfs_file_t *file = fisopfs_find(path + 1);
	if (!file)
		return -ENOENT;

	if (size > MAX_CONTENT_LEN)
		return -ENOMEM;

	file->st.st_size = size;

	time_t current_time;
	time(&current_time);
	file->st.st_mtime = current_time;
	file->st.st_ctime = current_time;

	return 0;
}


static struct fuse_operations operations = { .getattr = fisopfs_getattr,
	                                     .readdir = fisopfs_readdir,
	                                     .read = fisopfs_read,
	                                     .write = fisopfs_write,
	                                     .utimens = fisopfs_utimens,
	                                     .mknod = fisopfs_mknod,
	                                     .mkdir = fisopfs_mkdir,
	                                     .unlink = fisopfs_unlink,
	                                     .rmdir = fisopfs_rmdir,
	                                     .init = fisopfs_init,
	                                     .flush = fisopfs_flush,
	                                     .truncate = fisopfs_truncate,
	                                     .destroy = fisopfs_destroy };


int
main(int argc, char *argv[])
{
	char path[MAX_NAME_LEN];

	if (argc < 2)
		exit(1);

	bool is_disk_path = false;
	char *disk_path = "fs";
	char *fuse_argv[argc];
	int fuse_argc = 0;

	for (int i = 0; i < argc; i++) {
		if (is_disk_path) {
			disk_path = argv[i];
			is_disk_path = false;
			continue;
		}
		if (strcmp(argv[i], "-n") == 0) {
			// Indica que el proximo argumento es el path para persistir
			is_disk_path = true;
			continue;
		}
		fuse_argv[fuse_argc] = argv[i];
		fuse_argc++;
	}

	sprintf(path, "./%s.fisopfs", disk_path);

	fisopfs_sb_t sb = { .disk_path = path, .root = NULL, .size = 0 };

	fisopfs_sb_t *psb = malloc(sizeof(fisopfs_sb_t));
	*psb = sb;

	return fuse_main(fuse_argc, fuse_argv, &operations, psb);
}

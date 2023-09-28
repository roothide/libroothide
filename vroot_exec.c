#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <spawn.h>
#include <paths.h>
#include <sys/stat.h>
#include <sys/param.h>

#include "common.h"
#include "roothide.h"

char** environ;

#define VROOT_API_NAME(NAME) vroot_##NAME
#define VROOTAT_API_NAME(NAME) vroot_##NAME

int VROOT_API_NAME(stat)(const char *path, struct stat *sb);
int VROOT_API_NAME(execve)(const char * __file, char * const * __argv, char * const * __envp);
int VROOT_API_NAME(posix_spawn)(pid_t * pid, const char * path, const posix_spawn_file_actions_t *file_actions,
    const posix_spawnattr_t * attrp, char *const argv[], char *const envp[]);


#define	_write	write
#define	_execve	VROOT_API_NAME(execve)
int _execvpe(const char *name, char * const argv[], char * const envp[]);

EXPORT
int VROOT_API_NAME(execl)(const char *name, const char *arg, ...)
{
VROOT_LOG("@%s\n",__FUNCTION__);

	va_list ap;
	const char **argv;
	int n;

	va_start(ap, arg);
	n = 1;
	while (va_arg(ap, char *) != NULL)
		n++;
	va_end(ap);
	argv = alloca((n + 1) * sizeof(*argv));
	if (argv == NULL) {
		errno = ENOMEM;
		return (-1);
	}
	va_start(ap, arg);
	n = 1;
	argv[0] = arg;
	while ((argv[n] = va_arg(ap, char *)) != NULL)
		n++;
	va_end(ap);
	return (_execve(name, __DECONST(char **, argv), environ));
}

EXPORT
int VROOT_API_NAME(execle)(const char *name, const char *arg, ...)
{
VROOT_LOG("@%s\n",__FUNCTION__);

	va_list ap;
	const char **argv;
	char **envp;
	int n;

	va_start(ap, arg);
	n = 1;
	while (va_arg(ap, char *) != NULL)
		n++;
	va_end(ap);
	argv = alloca((n + 1) * sizeof(*argv));
	if (argv == NULL) {
		errno = ENOMEM;
		return (-1);
	}
	va_start(ap, arg);
	n = 1;
	argv[0] = arg;
	while ((argv[n] = va_arg(ap, char *)) != NULL)
		n++;
	envp = va_arg(ap, char **);
	va_end(ap);
	return (_execve(name, __DECONST(char **, argv), envp));
}

EXPORT
int VROOT_API_NAME(execvp)(const char *name, char * const *argv)
{
VROOT_LOG("@%s\n",__FUNCTION__);

	return (_execvpe(name, argv, environ));
}

EXPORT
int VROOT_API_NAME(execlp)(const char *name, const char *arg, ...)
{
VROOT_LOG("@%s\n",__FUNCTION__);

	va_list ap;
	const char **argv;
	int n;

	va_start(ap, arg);
	n = 1;
	while (va_arg(ap, char *) != NULL)
		n++;
	va_end(ap);
	argv = alloca((n + 1) * sizeof(*argv));
	if (argv == NULL) {
		errno = ENOMEM;
		return (-1);
	}
	va_start(ap, arg);
	n = 1;
	argv[0] = arg;
	while ((argv[n] = va_arg(ap, char *)) != NULL)
		n++;
	va_end(ap);
	return (VROOT_API_NAME(execvp)(name, __DECONST(char **, argv)));
}

EXPORT
int VROOT_API_NAME(execv)(const char *name, char * const *argv)
{
VROOT_LOG("@%s\n",__FUNCTION__);

	(void)_execve(name, argv, environ);
	return (-1);
}

static int
execvPe(const char *name, const char *path, char * const *argv,
    char * const *envp)
{
	const char **memp;
	size_t cnt, lp, ln;
	int eacces, save_errno;
	char *cur, buf[MAXPATHLEN];
	const char *p, *bp;
	struct stat sb;

	eacces = 0;

	/* If it's an absolute or relative path name, it's easy. */
	if (strchr(name, '/')) {
		bp = name;
		cur = NULL;
		goto retry;
	}
	bp = buf;

	/* If it's an empty path name, fail in the usual POSIX way. */
	if (*name == '\0') {
		errno = ENOENT;
		return (-1);
	}

	cur = alloca(strlen(path) + 1);
	if (cur == NULL) {
		errno = ENOMEM;
		return (-1);
	}
	strcpy(cur, path);
	while ((p = strsep(&cur, ":")) != NULL) {
		/*
		 * It's a SHELL path -- double, leading and trailing colons
		 * mean the current directory.
		 */
		if (*p == '\0') {
			p = ".";
			lp = 1;
		} else
			lp = strlen(p);
		ln = strlen(name);

		/*
		 * If the path is too long complain.  This is a possible
		 * security issue; given a way to make the path too long
		 * the user may execute the wrong program.
		 */
		if (lp + ln + 2 > sizeof(buf)) {
			(void)_write(STDERR_FILENO, "execvP: ", 8);
			(void)_write(STDERR_FILENO, p, lp);
			(void)_write(STDERR_FILENO, ": path too long\n",
			    16);
			continue;
		}
		bcopy(p, buf, lp);
		buf[lp] = '/';
		bcopy(name, buf + lp + 1, ln);
		buf[lp + ln + 1] = '\0';

retry:		(void)_execve(bp, argv, envp);
		switch (errno) {
		case E2BIG:
			goto done;
		case ELOOP:
		case ENAMETOOLONG:
		case ENOENT:
			break;
		case ENOEXEC:
			for (cnt = 0; argv[cnt]; ++cnt)
				;

			/*
			 * cnt may be 0 above; always allocate at least
			 * 3 entries so that we can at least fit "sh", bp, and
			 * the NULL terminator.  We can rely on cnt to take into
			 * account the NULL terminator in all other scenarios,
			 * as we drop argv[0].
			 */
			memp = alloca(MAX(3, cnt + 2) * sizeof(char *));
			if (memp == NULL) {
				/* errno = ENOMEM; XXX override ENOEXEC? */
				goto done;
			}
			if (cnt > 0) {
				memp[0] = argv[0];
				memp[1] = bp;
				bcopy(argv + 1, memp + 2, cnt * sizeof(char *));
			} else {
				memp[0] = "sh";
				memp[1] = bp;
				memp[2] = NULL;
			}
 			(void)_execve(_PATH_BSHELL,
			    __DECONST(char **, memp), envp);
			goto done;
		case ENOMEM:
			goto done;
		case ENOTDIR:
			break;
		case ETXTBSY:
			/*
			 * We used to retry here, but sh(1) doesn't.
			 */
			goto done;
		default:
			/*
			 * EACCES may be for an inaccessible directory or
			 * a non-executable file.  Call stat() to decide
			 * which.  This also handles ambiguities for EFAULT
			 * and EIO, and undocumented errors like ESTALE.
			 * We hope that the race for a stat() is unimportant.
			 */
			save_errno = errno;
			if (VROOT_API_NAME(stat)(bp, &sb) != 0)
				break;
			if (save_errno == EACCES) {
				eacces = 1;
				continue;
			}
			errno = save_errno;
			goto done;
		}
	}
	if (eacces)
		errno = EACCES;
	else if (cur)
		errno = ENOENT;
	/* else use existing errno from _execve */
done:
	return (-1);
}

EXPORT
int VROOT_API_NAME(execvP)(const char *name, const char *path, char * const argv[])
{
VROOT_LOG("@%s\n",__FUNCTION__);

	return execvPe(name, path, argv, environ);
}

__private_extern__ int
_execvpe(const char *name, char * const argv[], char * const envp[])
{
	const char *path;

	/* Get the path we're searching. */
	if ((path = getenv("PATH")) == NULL)
		path = _PATH_DEFPATH;

	return (execvPe(name, path, argv, envp));
}



EXPORT
int VROOT_API_NAME(posix_spawnp)(pid_t * pid, const char * file, const posix_spawn_file_actions_t *file_actions,
    const posix_spawnattr_t * attrp, char *const argv[], char *const envp[])
{
VROOT_LOG("@%s\n",__FUNCTION__);

	const char *env_path;
	char *bp;
	char *cur;
	char *p;
	char **memp;
	int lp;
	int ln;
	int cnt;
	int err = 0;
	int eacces = 0;
	struct stat sb;
	char path_buf[PATH_MAX];

	if ((env_path = getenv("PATH")) == NULL)
		env_path = _PATH_DEFPATH;

	/* If it's an absolute or relative path name, it's easy. */
	if (strchr(file, '/')) {
		bp = (char *)file;
		cur = NULL;
		goto retry;
	}
	bp = path_buf;

	/* If it's an empty path name, fail in the usual POSIX way. */
	if (*file == '\0')
		return (ENOENT);

	if ((cur = alloca(strlen(env_path) + 1)) == NULL)
		return ENOMEM;
	strcpy(cur, env_path);
	while ((p = strsep(&cur, ":")) != NULL) {
		/*
		 * It's a SHELL path -- double, leading and trailing colons
		 * mean the current directory.
		 */
		if (*p == '\0') {
			p = ".";
			lp = 1;
		} else {
			lp = strlen(p);
		}
		ln = strlen(file);

		/*
		 * If the path is too long complain.  This is a possible
		 * security issue; given a way to make the path too long
		 * the user may spawn the wrong program.
		 */
		if (lp + ln + 2 > sizeof(path_buf)) {
			err = ENAMETOOLONG;
			goto done;
		}
		bcopy(p, path_buf, lp);
		path_buf[lp] = '/';
		bcopy(file, path_buf + lp + 1, ln);
		path_buf[lp + ln + 1] = '\0';

retry:		err = VROOT_API_NAME(posix_spawn)(pid, bp, file_actions, attrp, argv, envp);
		switch (err) {
		case E2BIG:
		case ENOMEM:
		case ETXTBSY:
			goto done;
		case ELOOP:
		case ENAMETOOLONG:
		case ENOENT:
		case ENOTDIR:
			break;
		case ENOEXEC:
			for (cnt = 0; argv[cnt]; ++cnt)
				;

			/*
			 * cnt may be 0 above; always allocate at least
			 * 3 entries so that we can at least fit "sh", bp, and
			 * the NULL terminator.  We can rely on cnt to take into
			 * account the NULL terminator in all other scenarios,
			 * as we drop argv[0].
			 */
			memp = alloca(MAX(3, cnt + 2) * sizeof(char *));
			if (memp == NULL) {
				/* errno = ENOMEM; XXX override ENOEXEC? */
				goto done;
			}
			if (cnt > 0) {
				memp[0] = argv[0];
				memp[1] = bp;
				bcopy(argv + 1, memp + 2, cnt * sizeof(char *));
			} else {
				memp[0] = "sh";
				memp[1] = bp;
				memp[2] = NULL;
			}
			err = VROOT_API_NAME(posix_spawn)(pid, _PATH_BSHELL, file_actions, attrp, memp, envp);
			goto done;
		default:
			/*
			 * EACCES may be for an inaccessible directory or
			 * a non-executable file.  Call stat() to decide
			 * which.  This also handles ambiguities for EFAULT
			 * and EIO, and undocumented errors like ESTALE.
			 * We hope that the race for a stat() is unimportant.
			 */
			if (VROOT_API_NAME(stat)(bp, &sb) != 0)
				break;
			if (err == EACCES) {
				eacces = 1;
				continue;
			}
			goto done;
		}
	}
	if (eacces)
		err = EACCES;
	else
		err = ENOENT;
done:
	return (err);
}

//SPDX-License-Identifier: GPL-2.0-or-later
/*

   Copyright (c) 2019-2024 Cyril Hrubis <metan@ucw.cz>

 */

#include <shadow.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "login.h"

int neko_check_login(const char *username, const char *passwd)
{
	struct spwd *pwd = getspnam(username);

	if (!pwd) {
		perror("getspnam()");
		return 0;
	}

	const char *enc_passwd = pwd->sp_pwdp;
	const char *enc_typed_passwd = crypt(passwd, enc_passwd);

	if (!enc_typed_passwd)
		return 0;

	return !strcmp(enc_typed_passwd, enc_passwd);
}

int neko_switch_user(const char *username)
{
	struct passwd *pwd = getpwnam(username);

	if (!pwd)
		return 1;

	char *path = strdup(getenv("PATH"));

	if (setgid(pwd->pw_gid))
		return 1;

	if (initgroups(pwd->pw_name, pwd->pw_gid))
		return 1;

	if (setuid(pwd->pw_uid))
		return 1;

	setenv("HOME", pwd->pw_dir, 1);
	setenv("USER", pwd->pw_name, 1);
	setenv("LOGNAME", pwd->pw_name, 1);
	setenv("SHELL", pwd->pw_shell[0] ? pwd->pw_shell : "/bin/sh", 1);

	setenv("PATH", path, 1);
	free(path);

	if (chdir(pwd->pw_dir)) {
	}

	return 0;
}

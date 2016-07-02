#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include "daemon.h"


/* Pass a signal number and a signal handling function into this function
 * to handle signals sent to the process.
 */
signal_func *daemon_signal_handler(int signo, signal_func *func)
{
	struct sigaction act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (signo == SIGALRM) {
#ifdef SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;	/* SunOS 4.x */
#endif
	} else {
#ifdef SA_RESTART
		act.sa_flags |= SA_RESTART;	/* SVR4, 4.4BSD */
#endif
	}

	if (sigaction(signo, &act, &oact) < 0)
		return SIG_ERR;

	return oact.sa_handler;
}



/* Fork a child process and then kill the parent so make the calling
 * program a daemon process.
 */
void daemon_make(void)
{
	if (fork() != 0)
		exit(0);

	setsid();
	daemon_signal_handler(SIGHUP, SIG_IGN);

	if (fork() != 0)
		exit(0);

	chdir("/");
	umask(077);

#ifndef WITH_DEBUG
    /* When not in debugging mode, close the standard file
     * descriptors.
     */
	close(0);
	close(1);
	close(2);
#endif
}




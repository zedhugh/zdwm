#include <glib-unix.h>
#include <glib.h>
#include <glibconfig.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>

#include "backtrace.h"
#include "buffer.h"
#include "types.h"
#include "utils.h"

global_state_t global_state;

/** argv used to run wm */
static char **cmd_argv;

/** A pipe that is used to asynchronously handle SIGCHLD */
static int sigchld_pipe[2];

static void wm_atexit(bool restart) { global_state.need_restart = restart; }

static void wm_restart(void) {
  wm_atexit(true);
  execvp(cmd_argv[0], cmd_argv);
  fatal("execv() failed: %s", strerror(errno));
}

static gboolean restart_on_signal(gpointer data) {
  wm_restart();
  return TRUE;
}

static gboolean exit_on_signal(gpointer data) {
  g_main_loop_quit(global_state.loop);
  return TRUE;
}

static void signal_fatal(int signal_number) {
  buffer_t buffer;
  backtrace_get(&buffer);
  fatal("signal %d, dumping backtrace\n%s", signal_number, buffer.s);
}

/* Signal handler for SIGCHLD. Causes reap_children() to be called. */
static void signal_child(int signum) {
  assert(signum == SIGCHLD);
  int res = write(sigchld_pipe[1], " ", 1);
  (void)res;
  assert(res == 1);
}

static void check_other_wm(void) {
  /* connect X server */
  int default_screen;
  xcb_connection_t *conn = xcb_connect(NULL, &default_screen);
  int xcb_conn_error = xcb_connection_has_error(conn);
  if (xcb_conn_error) fatal("cannot open display (error %d)", xcb_conn_error);

  xcb_screen_t *screen = xcb_aux_get_screen(conn, default_screen);
  xcb_window_t root = screen->root;

  /* check other window manager running */
  uint32_t mask = XCB_CW_EVENT_MASK;
  const xcb_params_cw_t params = {
    .event_mask = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
  };
  xcb_void_cookie_t cookie =
    xcb_aux_change_window_attributes_checked(conn, root, mask, &params);
  if (xcb_request_check(conn, cookie)) {
    fatal(
      "another window manager is already running (cannot select "
      "SubstructureRedirect)");
  }

  global_state.xcb_conn = conn;
  global_state.default_screen = default_screen;
  global_state.screen = screen;
}

int main(int argc, char *argv[]) {
  p_clear(&global_state, 1);

  cmd_argv = argv;

  g_unix_signal_add(SIGINT, exit_on_signal, NULL);
  g_unix_signal_add(SIGTERM, exit_on_signal, NULL);
  g_unix_signal_add(SIGHUP, restart_on_signal, NULL);

  struct sigaction sa = {.sa_handler = signal_fatal, .sa_flags = SA_RESETHAND};
  sigemptyset(&sa.sa_mask);
  sigaction(SIGABRT, &sa, 0);
  sigaction(SIGBUS, &sa, 0);
  sigaction(SIGFPE, &sa, 0);
  sigaction(SIGILL, &sa, 0);
  sigaction(SIGSEGV, &sa, 0);
  signal(SIGPIPE, SIG_IGN);

  sa.sa_handler = signal_child;
  sa.sa_flags = SA_NOCLDSTOP | SA_RESTART;
  sigaction(SIGCHLD, &sa, 0);

  check_other_wm();

  if (global_state.loop == NULL) {
    global_state.loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(global_state.loop);
  }
  g_main_loop_unref(global_state.loop);
  global_state.loop = NULL;

  wm_atexit(false);

  return EXIT_SUCCESS;
}

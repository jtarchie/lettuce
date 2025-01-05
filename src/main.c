#include "redcon.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void *zmalloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr)
    abort();
  return ptr;
}

void serving(const char **addrs, int naddrs, void *udata) {
  printf("* Listening at %s\n", addrs[0]);
}

void error(const char *msg, bool fatal, void *udata) {
  fprintf(stderr, "- %s\n", msg);
}

void command(struct redcon_conn *conn, struct redcon_args *args, void *udata) {
  redcon_conn_write_null(conn);
}

int main() {
  // use do-or-die allocator
  redcon_set_allocator(zmalloc, free);

  struct redcon_events evs = {
      .serving = serving,
      .command = command,
      .error = error,
  };
  const char *addrs[] = {
      "tcp://0.0.0.0:6380",
  };
  redcon_main(addrs, 1, evs, NULL);
  return 0;
}
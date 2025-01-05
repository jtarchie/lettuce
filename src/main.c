#include "redcon.h"
#include "sqlite3.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

struct sqlite3 *db;

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

sqlite3_stmt *setStatement;

void cmdSET(struct redcon_conn *conn, struct redcon_args *args, void *udata) {
  if (redcon_args_count(args) != 3) {
    redcon_conn_write_error(conn, "ERR wrong number of arguments");
  } else {
    const char *key = redcon_args_at(args, 1, NULL);
    const char *value = redcon_args_at(args, 2, NULL);

    sqlite3_bind_text(setStatement, 1, key, -1, SQLITE_STATIC);
    sqlite3_bind_text(setStatement, 2, value, -1, SQLITE_STATIC);

    int rv = sqlite3_step(setStatement);
    if (rv != SQLITE_DONE) {
      fprintf(stderr, "sqlite3_step: %s\n", sqlite3_errmsg(db));
      redcon_conn_write_error(conn, sqlite3_errmsg(db));
    } else {
      redcon_conn_write_string(conn, "OK");
    }

    rv = sqlite3_reset(setStatement);
    if (rv != SQLITE_OK) {
      fprintf(stderr, "sqlite3_reset: %s\n", sqlite3_errmsg(db));
    }
  }
}

void command(struct redcon_conn *conn, struct redcon_args *args, void *udata) {
  if (redcon_args_eq(args, 0, "set")) {
    cmdSET(conn, args, udata);
  } else {
    fprintf(stderr, "unknown command: %s\n", redcon_args_at(args, 0, NULL));
    redcon_conn_write_error(conn, "ERR unknown command");
  }
}

int main() {
  int rv = sqlite3_open_v2(":memory:", &db,
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX, NULL);
  if (rv != SQLITE_OK) {
    fprintf(stderr, "sqlite3_open_v2: %s\n", sqlite3_errmsg(db));
    return 1;
  }

  rv = sqlite3_exec(db,
                    "CREATE TABLE keys("
                    "  id integer primary key autoincrement,"
                    "  key text not null unique,"
                    "  value text not null"
                    ");"
                    "pragma journal_mode = wal;"
                    "pragma synchronous = normal;"
                    "pragma temp_store = memory;"
                    "pragma mmap_size = 268435456;"
                    "pragma foreign_keys = on;",
                    NULL, NULL, NULL);
  if (rv != SQLITE_OK) {
    fprintf(stderr, "sqlite3_exec: %s\n", sqlite3_errmsg(db));
    return 1;
  }

  rv = sqlite3_prepare_v2(
      db, "INSERT OR REPLACE INTO keys(key, value) VALUES(?, ?);", -1,
      &setStatement, NULL);
  if (rv != SQLITE_OK) {
    fprintf(stderr, "sqlite3_prepare_v2: %s\n", sqlite3_errmsg(db));
    return 1;
  }

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
  redcon_main_mt(addrs, 1, evs, NULL, 1);
  return 0;
}
#include "atoms.h"

#include <xcb/xproto.h>

#include "atoms-intern.h"
#include "utils.h"

void atoms_init(xcb_connection_t *conn) {
  xcb_intern_atom_cookie_t cookies[countof(ATOM_LIST)];
  for (int i = 0; i < countof(ATOM_LIST); i++) {
    atom_item_t atom = ATOM_LIST[i];
    cookies[i] = xcb_intern_atom_unchecked(conn, false, atom.len, atom.name);
  }

  xcb_intern_atom_reply_t *reply = nullptr;
  for (int i = 0; i < countof(ATOM_LIST); i++) {
    reply = xcb_intern_atom_reply(conn, cookies[i], nullptr);
    if (reply) {
      *ATOM_LIST[i].atom = reply->atom;
      p_delete(&reply);
    }
  }
}

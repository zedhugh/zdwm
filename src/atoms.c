#include "atoms.h"

#include <stdint.h>
#include <string.h>
#include <xcb/xproto.h>

#include "atoms-intern.h"
#include "utils.h"
#include "wm.h"

void atoms_init(xcb_connection_t *conn) {
  xcb_intern_atom_cookie_t cookies[countof(ATOM_LIST)];
  for (int i = 0; i < countof(ATOM_LIST); i++) {
    atom_item_t atom = ATOM_LIST[i];
    cookies[i] = xcb_intern_atom_unchecked(conn, false, atom.len, atom.name);
  }

  atom_item_t *atom = nullptr;
  uint32_t supported_length = 0;
  xcb_atom_t supported_list[countof(ATOM_LIST)] = {XCB_ATOM_NONE};

  xcb_intern_atom_reply_t *reply = nullptr;
  for (int i = 0; i < countof(ATOM_LIST); i++) {
    reply = xcb_intern_atom_reply(conn, cookies[i], nullptr);
    if (reply) {
      atom = &ATOM_LIST[i];
      *atom->atom = reply->atom;
      p_delete(&reply);

      if (strstr(atom->name, "_NET_") == atom->name) {
        supported_list[supported_length++] = *atom->atom;
      }
    }
  }

  xcb_change_property(wm.xcb_conn, XCB_PROP_MODE_REPLACE, wm.screen->root,
                      _NET_SUPPORTED, XCB_ATOM_ATOM, 32, supported_length,
                      supported_list);
}

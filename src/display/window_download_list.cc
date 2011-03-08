// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#include "config.h"

#include <rak/algorithm.h>

#include "core/download.h"
#include "core/view.h"

#include "canvas.h"
#include "globals.h"
#include "utils.h"
#include "window_download_list.h"
#include "rpc/parse_commands.h"

namespace display {

WindowDownloadList::WindowDownloadList() :
  Window(new Canvas, 0, 120, 1, extent_full, extent_full),
  m_view(NULL) {
}

WindowDownloadList::~WindowDownloadList() {
  if (m_view != NULL)
    m_view->signal_changed().erase(m_changed_itr);
  
  m_view = NULL;
}

void
WindowDownloadList::set_view(core::View* l) {
  if (m_view != NULL)
    m_view->signal_changed().erase(m_changed_itr);

  m_view = l;

  if (m_view != NULL)
    m_changed_itr = m_view->signal_changed().insert(m_view->signal_changed().begin(), std::bind(&Window::mark_dirty, this));
}

void
WindowDownloadList::redraw() {
  m_slotSchedule(this, (cachedTime + rak::timer::from_seconds(1)).round_seconds());

  m_canvas->erase();

  if (m_view == NULL)
    return;

  m_canvas->print(0, 0, "%s", ("[View: " + m_view->name() + "]").c_str());

  if (m_view->empty_visible() || m_canvas->width() < 5 || m_canvas->height() < 2)
    return;

  typedef std::pair<core::View::iterator, core::View::iterator> Range;

  if (rpc::call_command_value("ui.wideui") != 0) {
    const int info_width = 85;
    const int title_width = std::max(std::min(static_cast<int>(m_canvas->width()) - info_width, 60), 5); // max=60, min=5

    Range range = rak::advance_bidirectional(m_view->begin_visible(),
                                             m_view->focus() != m_view->end_visible() ? m_view->focus() : m_view->begin_visible(),
                                             m_view->end_visible(),
                                             m_canvas->height() - 2);

    m_canvas->print(0, 1, "Torrent name");
    m_canvas->print(title_width, 1, "      dl-ed      all   %%      up    down     up-ed  ratio     peers      left info");
    m_canvas->print(title_width + info_width, 1, "status");

    int pos = 2;

    while (range.first != range.second) {

      m_canvas->print(0, pos, "%c%s", range.first == m_view->focus() ? '*' : ' ', (*range.first)->info()->name().c_str());

      char info[info_width];
      print_download_info_wide(info, info + info_width, *range.first);
      m_canvas->print(title_width, pos, "%s", info);

      if (title_width + info_width < m_canvas->width()) { // check space for status
        const int status_width = 200;
        char status[status_width];
        print_download_status(status, status + status_width, *range.first);
        if (title_width + info_width + status_width > m_canvas->width()) // truncate status
            status[m_canvas->width() - title_width - info_width] = '\0';
        m_canvas->print(title_width + info_width, pos, "%s", status);
      }

      if (range.first == m_view->focus())
        m_canvas->set_attr(0, pos, m_canvas->width(), A_UNDERLINE, 0);
      else
        m_canvas->set_attr(0, pos, m_canvas->width(), A_NORMAL, 0);

      pos++;
      ++range.first;
    }
  }
  else {

    Range range = rak::advance_bidirectional(m_view->begin_visible(),
                                             m_view->focus() != m_view->end_visible() ? m_view->focus() : m_view->begin_visible(),
                                             m_view->end_visible(),
                                             m_canvas->height() / 3);

    // Make sure we properly fill out the last lines so it looks like
    // there are more torrents, yet don't hide it if we got the last one
    // in focus.
    if (range.second != m_view->end_visible())
      ++range.second;

    int pos = 1;

    while (range.first != range.second) {
      char buffer[m_canvas->width() + 1];
      char* position;
      char* last = buffer + m_canvas->width() - 2 + 1;

      position = print_download_title(buffer, last, *range.first);
      m_canvas->print(0, pos++, "%c %s", range.first == m_view->focus() ? '*' : ' ', buffer);

      position = print_download_info(buffer, last, *range.first);
      m_canvas->print(0, pos++, "%c %s", range.first == m_view->focus() ? '*' : ' ', buffer);

      position = print_download_status(buffer, last, *range.first);
      m_canvas->print(0, pos++, "%c %s", range.first == m_view->focus() ? '*' : ' ', buffer);

      ++range.first;
    }
  }
}

}

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef WAF_BUILD
#include "gtk2ardour-config.h"
#endif

#include <algorithm>
#include <map>

#include <ytkmm/box.h>
#include <ytkmm/cellrenderertext.h>
#include <ytkmm/toggleaction.h>
#include <ytkmm/treeviewcolumn.h>

#include "pbd/replace_all.h"

#include "gtkmm2ext/actions.h"
#include "gtkmm2ext/bindings.h"

#include "ardour_ui.h"
#include "command_palette.h"
#include "ui_config.h"

#include "pbd/i18n.h"

using namespace std;
using namespace Gtk;
using namespace Gtkmm2ext;

CommandPalette* CommandPalette::_instance = 0;

void
CommandPalette::show_palette ()
{
	if (!_instance) {
		_instance = new CommandPalette ();
	}

	Gtk::Window& main_window = ARDOUR_UI::instance()->main_window ();

	_instance->set_transient_for (main_window);
	_instance->collect_actions ();
	_instance->_entry.set_text ("");
	_instance->rebuild_list ();

	/* near the top, centered over the main window */
	int px, py, pw, ph;
	int w, h;
	main_window.get_position (px, py);
	main_window.get_size (pw, ph);
	_instance->get_default_size (w, h);
	_instance->move (px + max (0, (pw - w) / 2), py + ph / 8);

	_instance->present ();
	_instance->_entry.grab_focus ();
}

CommandPalette::CommandPalette ()
	: Gtk::Window (Gtk::WINDOW_TOPLEVEL)
{
	set_title (_("Find Command"));
	set_default_size (600, 420);
	set_type_hint (Gdk::WINDOW_TYPE_HINT_DIALOG);
	set_skip_taskbar_hint (true);
	set_border_width (8);

	_store = ListStore::create (_columns);
	_tree.set_model (_store);
	_tree.set_headers_visible (false);
	_tree.set_can_focus (false);
	_tree.set_enable_search (false);
	_tree.get_selection()->set_mode (SELECTION_SINGLE);

	/* The cells would otherwise use the toolkit's default font, not the UI font. */
	Pango::FontDescription const fd = UIConfiguration::instance().get_NormalFont ();
	CellRendererText* label_cell = manage (new CellRendererText ());
	label_cell->property_font_desc () = fd;
	label_cell->property_xpad () = 6;
	label_cell->property_ellipsize () = Pango::ELLIPSIZE_END;

	_group_cell.property_font_desc () = fd;
	_group_cell.property_xpad () = 6;

	_shortcut_cell.property_font_desc () = fd;
	_shortcut_cell.property_xpad () = 6;
	_shortcut_cell.property_xalign () = 1.0;

	TreeViewColumn* col;

	col = manage (new TreeViewColumn ());
	col->pack_start (*label_cell, true);
	col->add_attribute (label_cell->property_text (), _columns.label);
	col->add_attribute (label_cell->property_sensitive (), _columns.sensitive);
	col->set_expand (true);
	_tree.append_column (*col);

	col = manage (new TreeViewColumn ());
	col->pack_start (_group_cell, false);
	col->add_attribute (_group_cell.property_text (), _columns.group);
	col->add_attribute (_group_cell.property_sensitive (), _columns.sensitive);
	_tree.append_column (*col);

	col = manage (new TreeViewColumn ());
	col->pack_start (_shortcut_cell, false);
	col->add_attribute (_shortcut_cell.property_text (), _columns.shortcut);
	col->add_attribute (_shortcut_cell.property_sensitive (), _columns.sensitive);
	_tree.append_column (*col);

	_scroller.set_policy (POLICY_NEVER, POLICY_AUTOMATIC);
	_scroller.set_shadow_type (SHADOW_IN);
	_scroller.add (_tree);

	VBox* vbox = manage (new VBox ());
	vbox->set_spacing (8);
	vbox->pack_start (_entry, false, false);
	vbox->pack_start (_scroller, true, true);
	add (*vbox);
	vbox->show_all ();

	_entry.signal_changed().connect (sigc::mem_fun (*this, &CommandPalette::query_changed));
	_tree.signal_row_activated().connect (sigc::mem_fun (*this, &CommandPalette::row_activated));
}

void
CommandPalette::on_realize ()
{
	Gtk::Window::on_realize ();
	update_colors ();
}

void
CommandPalette::update_colors ()
{
	/* the group is secondary information: draw it in the dimmed text color */
	_tree.ensure_style ();
	Gdk::Color const dim = _tree.get_style()->get_text (STATE_INSENSITIVE);
	_group_cell.property_foreground_gdk () = dim;
}

string
CommandPalette::strip_mnemonic (string const& s)
{
	string r;
	r.reserve (s.size ());

	for (string::size_type i = 0; i < s.size (); ++i) {
		if (s[i] == '_') {
			if (i + 1 < s.size () && s[i + 1] == '_') {
				/* escaped underscore */
				r += '_';
				++i;
			}
			continue;
		}
		r += s[i];
	}

	return r;
}

void
CommandPalette::collect_actions ()
{
	_entries.clear ();

	vector<string> paths;
	vector<string> labels;
	vector<string> tooltips;
	vector<string> keys;
	vector<Glib::RefPtr<Gtk::Action> > actions;

	ActionManager::get_all_actions (paths, labels, tooltips, keys, actions);

	/* Each Bindings only reports the keys of the actions it owns, so go
	 * through all of them (as the keyboard shortcut editor does).
	 */
	std::map<Gtk::Action*, string> shortcuts;

	for (list<Bindings*>::const_iterator b = Bindings::bindings.begin (); b != Bindings::bindings.end (); ++b) {
		vector<string> b_paths;
		vector<string> b_labels;
		vector<string> b_tooltips;
		vector<string> b_keys;
		vector<Glib::RefPtr<Gtk::Action> > b_actions;

		(*b)->get_all_actions (b_paths, b_labels, b_tooltips, b_keys, b_actions);

		for (size_t i = 0; i < b_actions.size (); ++i) {
			if (b_keys[i].empty () || shortcuts.find (b_actions[i].operator->()) != shortcuts.end ()) {
				continue;
			}
			string s = b_keys[i];
			replace_all (s, "<", "");
			replace_all (s, ">", "-");
			shortcuts[b_actions[i].operator->()] = s;
		}
	}

	/* the same label can be registered twice (e.g. alternate key actions); keep the one with a shortcut */
	std::map<string, size_t> seen;

	for (size_t i = 0; i < actions.size (); ++i) {

		Glib::RefPtr<Gtk::Action> act = actions[i];

		if (!act) {
			continue;
		}

		string label = strip_mnemonic (labels[i]);

		if (label.empty ()) {
			continue;
		}

		/* plain actions without a handler are menu headers: nothing to run */
		if (!Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic (act) &&
		    !g_signal_has_handler_pending (G_OBJECT (act->gobj ()), g_signal_lookup ("activate", GTK_TYPE_ACTION), 0, TRUE)) {
			continue;
		}

		string group;
		string::size_type slash = paths[i].find ('/');
		if (slash != string::npos) {
			group = paths[i].substr (0, slash);
		}

		Item e;
		e.label     = label;
		e.group     = group;
		e.action    = act;
		e.sensitive = act->is_sensitive ();

		std::map<Gtk::Action*, string>::const_iterator s = shortcuts.find (act.operator->());
		if (s != shortcuts.end ()) {
			e.shortcut = s->second;
		}

		Glib::ustring const lower = Glib::ustring (label + " " + group).lowercase ();
		e.haystack = lower;
		e.sort_key = Glib::ustring (label).lowercase ();

		string const key = group + "/" + label;
		std::map<string, size_t>::const_iterator d = seen.find (key);

		if (d != seen.end ()) {
			if (_entries[d->second].shortcut.empty () && !e.shortcut.empty ()) {
				_entries[d->second] = e;
			}
			continue;
		}

		seen[key] = _entries.size ();
		_entries.push_back (e);
	}
}

void
CommandPalette::query_changed ()
{
	rebuild_list ();
}

void
CommandPalette::rebuild_list ()
{
	/* every word of the query must occur in the label or group */
	vector<string> words;
	{
		string const q = Glib::ustring (_entry.get_text ()).lowercase ();
		string::size_type pos = 0;
		while (pos < q.size ()) {
			string::size_type end = q.find_first_of (" \t", pos);
			if (end == string::npos) {
				end = q.size ();
			}
			if (end > pos) {
				words.push_back (q.substr (pos, end - pos));
			}
			pos = end + 1;
		}
	}

	/* rank: label starts with the first word, then a word of the label does, then any other match;
	 * actions that cannot be run now come last.
	 */
	typedef std::pair<int, unsigned int> Ranked; /* (rank, index into _entries) */
	vector<Ranked> matches;

	for (unsigned int i = 0; i < _entries.size (); ++i) {

		Item const& e = _entries[i];
		bool all = true;

		for (vector<string>::const_iterator w = words.begin (); w != words.end (); ++w) {
			if (e.haystack.find (*w) == string::npos) {
				all = false;
				break;
			}
		}

		if (!all) {
			continue;
		}

		int rank = 2;

		if (!words.empty ()) {
			if (e.sort_key.compare (0, words.front().size (), words.front ()) == 0) {
				rank = 0;
			} else if (e.sort_key.find (" " + words.front ()) != string::npos) {
				rank = 1;
			}
		}

		if (!e.sensitive) {
			rank += 3;
		}

		matches.push_back (Ranked (rank, i));
	}

	sort (matches.begin (), matches.end (), [this] (Ranked const& a, Ranked const& b) {
		if (a.first != b.first) {
			return a.first < b.first;
		}
		return _entries[a.second].sort_key < _entries[b.second].sort_key;
	});

	_tree.unset_model ();
	_store->clear ();

	for (vector<Ranked>::const_iterator m = matches.begin (); m != matches.end (); ++m) {
		Item const& e = _entries[m->second];
		TreeModel::Row row = *(_store->append ());
		row[_columns.label]     = e.label;
		row[_columns.group]     = e.group;
		row[_columns.shortcut]  = e.shortcut;
		row[_columns.index]     = m->second;
		row[_columns.sensitive] = e.sensitive;
	}

	_tree.set_model (_store);

	if (!_store->children().empty ()) {
		_tree.get_selection()->select (_store->children().begin ());
		_tree.scroll_to_row (TreeModel::Path ("0"), 0.0);
	}
}

void
CommandPalette::move_selection (int delta)
{
	int const n = _store->children().size ();

	if (n == 0) {
		return;
	}

	int current = -1;
	TreeModel::iterator sel = _tree.get_selection()->get_selected ();

	if (sel) {
		current = _store->get_path (sel)[0];
	}

	int const next = max (0, min (n - 1, current + delta));

	TreeModel::Path path;
	path.push_back (next);
	_tree.get_selection()->select (path);
	_tree.scroll_to_row (path);
}

void
CommandPalette::activate_selected ()
{
	TreeModel::iterator sel = _tree.get_selection()->get_selected ();

	if (!sel) {
		return;
	}

	Item const& e = _entries[(*sel)[_columns.index]];

	if (!e.action || !e.action->is_sensitive ()) {
		return;
	}

	Glib::RefPtr<Gtk::Action> act = e.action;

	/* let the main window take the focus back before the action runs */
	hide ();
	Glib::signal_idle().connect_once ([act] () { act->activate (); });
}

void
CommandPalette::row_activated (TreeModel::Path const&, TreeViewColumn*)
{
	activate_selected ();
}

bool
CommandPalette::on_key_press_event (GdkEventKey* ev)
{
	switch (ev->keyval) {
	case GDK_Escape:
		hide ();
		return true;
	case GDK_Up:
	case GDK_KP_Up:
		move_selection (-1);
		return true;
	case GDK_Down:
	case GDK_KP_Down:
		move_selection (1);
		return true;
	case GDK_Page_Up:
	case GDK_KP_Page_Up:
		move_selection (-10);
		return true;
	case GDK_Page_Down:
	case GDK_KP_Page_Down:
		move_selection (10);
		return true;
	case GDK_Return:
	case GDK_KP_Enter:
	case GDK_ISO_Enter:
		activate_selected ();
		return true;
	default:
		break;
	}

	return Gtk::Window::on_key_press_event (ev);
}

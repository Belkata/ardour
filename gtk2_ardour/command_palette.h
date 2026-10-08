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

#pragma once

#include <string>
#include <vector>

#include <ytkmm/action.h>
#include <ytkmm/entry.h>
#include <ytkmm/liststore.h>
#include <ytkmm/scrolledwindow.h>
#include <ytkmm/treeview.h>
#include <ytkmm/window.h>

/** A searchable list of every action, opened with a keyboard shortcut. */
class CommandPalette : public Gtk::Window
{
public:
	/** Open the palette (creating it on first use) and rebuild its list. */
	static void show_palette ();

protected:
	bool on_key_press_event (GdkEventKey*);
	void on_realize ();

private:
	CommandPalette ();

	struct Item {
		std::string                label;     /* without mnemonic underscores */
		std::string                group;
		std::string                shortcut;
		std::string                haystack;  /* lower case label + group, for matching */
		std::string                sort_key;  /* lower case label */
		Glib::RefPtr<Gtk::Action>  action;
		bool                       sensitive;
	};

	struct Columns : public Gtk::TreeModel::ColumnRecord {
		Columns () {
			add (label);
			add (group);
			add (shortcut);
			add (index);
			add (sensitive);
		}
		Gtk::TreeModelColumn<std::string> label;
		Gtk::TreeModelColumn<std::string> group;
		Gtk::TreeModelColumn<std::string> shortcut;
		Gtk::TreeModelColumn<unsigned int> index;  /* into _entries */
		Gtk::TreeModelColumn<bool>        sensitive;
	};

	void collect_actions ();
	void rebuild_list ();
	void query_changed ();
	void move_selection (int delta);
	void activate_selected ();
	void row_activated (Gtk::TreeModel::Path const&, Gtk::TreeViewColumn*);
	void update_colors ();

	static std::string strip_mnemonic (std::string const&);

	static CommandPalette* _instance;

	Columns                      _columns;
	Glib::RefPtr<Gtk::ListStore> _store;
	Gtk::Entry                   _entry;
	Gtk::TreeView                _tree;
	Gtk::ScrolledWindow          _scroller;
	Gtk::CellRendererText        _group_cell;
	Gtk::CellRendererText        _shortcut_cell;
	std::vector<Item>            _entries;
};

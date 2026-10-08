ardour { ["type"] = "SessionInit", name = "MIDI demo session" }
function factory () return function ()
	local pos = Temporal.timepos_t(0)
	for _, n in ipairs ({ "drums", "bass", "keys", "lead" }) do
		local files = C.StringVector()
		files:push_back (os.getenv ("ARDOUR_DEMO_MIDI") .. "/" .. n .. ".mid")
		Editor:do_import (files,
			Editing.ImportDistinctFiles, Editing.ImportAsTrack, ARDOUR.SrcQuality.SrcBest,
			ARDOUR.MidiTrackNameSource.SMFFileAndTrackName, ARDOUR.MidiTempoMapDisposition.SMFTempoIgnore,
			pos, ARDOUR.PluginInfo(), ARDOUR.Track(), false)
	end
	Session:save_state ("", false, false, false, false, false)
end end

ardour { ["type"] = "SessionInit", name = "UI demo session" }
function factory () return function ()
	local names = { "kick", "snare", "bass", "guitar", "vocal" }
	local pos = Temporal.timepos_t(0)
	for _, n in ipairs (names) do
		local files = C.StringVector()
		files:push_back (os.getenv ("ARDOUR_DEMO_AUDIO") .. "/" .. n .. ".wav")
		Editor:do_import (files,
			Editing.ImportDistinctFiles, Editing.ImportAsTrack, ARDOUR.SrcQuality.SrcBest,
			ARDOUR.MidiTrackNameSource.SMFFileAndTrackName, ARDOUR.MidiTempoMapDisposition.SMFTempoIgnore,
			pos, ARDOUR.PluginInfo(), ARDOUR.Track(), false)
	end
	Session:new_audio_route (2, 2, ARDOUR.RouteGroup (), 1, "Drum bus", ARDOUR.PresentationInfo.Flag.AudioBus, ARDOUR.PresentationInfo.max_order)
	Session:save_state ("", false, false, false, false, false)
end end

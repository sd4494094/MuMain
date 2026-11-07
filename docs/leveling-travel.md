# Leveling travel NPC

On servers with this optional service enabled, speak to **Travel Guide Noah** in
Lorencia at **143, 131**. The server supplies a scrollable list with the full map/floor
name, location description, your effective required level and the Zen price.

The compact list keeps full map and location names together, with requirements on the
next line. Only destinations scroll; the Close button stays visible below the list.

Click an enabled row to travel immediately. Close or Escape cancels without spending
anything and ends the NPC conversation. Locations are shared map positions, not private
rooms. This service leaves the ordinary **M** travel window unchanged.

Both the server and client must support the leveling-travel extension. The server
controls availability, conditions, fees and coordinates. Bot accounts cannot use this
NPC. If a destination or price changes while the list is open, reopen the conversation
to obtain the current choices.

## Town NPC presentation

The matching client data gives Erin (464) her own female robe model, while Leo (371)
keeps his excellent Black Dragon equipment and Storm wings in the flying idle pose,
including in town safe zones. These are cosmetic changes; NPC services keep their
server-side rules.

The Lorencia Tab map resource includes Noah, Leo, Erin at the tavern door (130,133),
and all four configured Phantom Helper locations. When server placements change,
update the localized minimap data as well. The private client package preserves all
portals and other markers and compacts empty records so later markers remain visible.

Operators can maintain a licensed minimap with `tools/update_minimap_npcs.py`,
passing separate `--input`, `--placements` and new `--output` paths. The placement JSON
lists names and all their tile coordinates. The tool checks the checksum, UTF-8
field lengths, duplicate placements and file capacity. Back up the installed file
and verify the markers against server spawns before publishing.

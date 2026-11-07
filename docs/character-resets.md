# Character reset NPC

On servers with the reset-dialog extension enabled, speaking to the configured
reset NPC opens a menu with its name, your current requirements and the result of
a reset. The server supplies the text and the rules in your character's language.

- **Query reset conditions** refreshes your level, zen, required items and the
  resulting level and points without spending anything.
- **Apply for reset** shows the conditions again with a **Confirm reset** button.
  Only that confirmation requests the reset.
- **Leave** or Escape closes the conversation.

A confirmation can be used only once and expires after two minutes. Reopening,
refreshing conditions or changes in the displayed requirements require a new
confirmation. Attributes, reset counts, fees, relocation and returning to character
selection follow the server configuration.

The menu supports both themes. Long requirements scroll while the action buttons
stay visible. It requires a server with the MuMain reset-dialog extension; an
ordinary Season 6 server does not send this menu. Stat resets remain a separate
NPC feature.

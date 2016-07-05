#ifndef __USERINTERFACE_H__
#define __USERINTERFACE_H__

class UserInterface
{
public:
	virtual ~UserInterface() {};

	virtual const char *Name() const = 0; // Returns the name of the gui.
	virtual const char *Comment() const = 0; // Returns a comment on the gui.
	virtual bool IsInteractive() const = 0; // Returns true if the gui is interactive.
	virtual bool IsUniqued() const = 0;
	virtual void SetUniqued(bool b) = 0;
	virtual bool InitFromFile(const char *path, bool rebuild = true, bool cache = true) = 0; // returns false if it failed to load
	virtual const char *HandleEvent(const Event_t *event, int time, bool *updateVisuals = nullptr) = 0; // handles an event, can return an action string, the caller interprets any return and acts accordingly
	virtual void HandleNamedEvent(const char *eventName) = 0; // handles a named event
	virtual void Redraw(int time, bool hud = false) = 0; // repaints the ui
	virtual void DrawCursor() = 0; // repaints the cursor

	// State management
	virtual const Dict &State() const = 0;
	virtual void DeleteState(const char *varName) = 0;
	virtual void SetState(const char *varName, const char *value) = 0;
	virtual void SetState(const char *varName, const bool value) = 0;
	virtual void SetState(const char *varName, const int value) = 0;
	virtual void SetState(const char *varName, const float value) = 0;
	virtual const char *GetStateString(const char *varName, const char *defaultString = "") const = 0;
	virtual bool GetStateBool(const char *varName, const char *defaultString = "0") const = 0;
	virtual int GetStateInt(const char *varName, const char *defaultString = "0") const = 0;
	virtual float GetStateFloat(const char *varName, const char *defaultString = "0") const = 0;
	virtual void StateChanged(int time, bool redraw = false) = 0; // The state has changed and the gui needs to update from the state Dict.

	virtual const char *Activate(bool activate, int time) = 0; // Activated the gui
	virtual void Trigger(int time) = 0; // Triggers the gui and runs the onTrigger scripts.
	virtual void SetKeyBindingNames() = 0;

	virtual void SetCursor(float x, float y) = 0;
	virtual float CursorX() = 0;
	virtual float CursorY() = 0;
};

class UserInterfaceManager
{
public:
	virtual ~UserInterfaceManager() {};

	virtual void Init() = 0;
	virtual void Shutdown() = 0;
	virtual void Touch(const char *name) = 0;
	//virtual void WritePrecacheCommands(File *f) = 0;

	virtual void SetDrawingDC() = 0;

	// Sets the size for 640x480 adjustment.
	virtual void SetSize(float width, float height) = 0;

	//virtual void BeginLevelLoad() = 0;
	//virtual void EndLevelLoad(const char *mapName) = 0;
	//virtual void Preload(const char *mapName) = 0;

	virtual void Reload(bool all) = 0; // Reloads changed guis, or all guis.
	virtual void ListGuis() const = 0; // lists all guis
	virtual bool CheckGui(const char *path) const = 0; // Returns true if gui exists.

	virtual UserInterface *Alloc() const = 0; // Allocates a new gui.
	virtual void DeAlloc(UserInterface *gui) = 0; // De-allocates a gui.. ONLY USE FOR PRECACHING
	virtual UserInterface *FindGui(const char *path, bool autoLoad = false, bool needUnique = false, bool forceUnique = false) = 0; // Returns NULL if gui by that name does not exist.
	virtual	ListGUI *AllocListGUI() const = 0; // Allocates a new GUI list handler
	virtual void FreeListGUI(ListGUI *listgui) = 0; // De-allocates a list gui
};

extern UserInterfaceManager *_uiManager;

#endif /* !__USERINTERFACE_H__ */

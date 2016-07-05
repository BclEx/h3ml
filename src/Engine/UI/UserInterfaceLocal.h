struct Event_t;
class Window;
class UserInterfaceLocal : public UserInterface
{
	friend class UserInterfaceManagerLocal;
public:
	UserInterfaceLocal();
	virtual ~UserInterfaceLocal();

	virtual const char *Name() const;
	virtual const char *Comment() const;
	virtual bool IsInteractive() const;
	virtual bool IsUniqued() const { return _uniqued; };
	virtual void SetUniqued(bool b) { _uniqued = b; };
	virtual bool InitFromFile(const char *path, bool rebuild = true, bool cache = true);
	virtual const char *HandleEvent(const Event_t *event, int time, bool *updateVisuals);
	virtual void HandleNamedEvent(const char *namedEvent);
	virtual void Redraw(int time, bool hud);
	virtual void DrawCursor();

	// State management
	virtual const Dict &State() const;
	virtual void DeleteState(const char *varName);
	virtual void SetState(const char *varName, const char *value);
	virtual void SetState(const char *varName, const bool value);
	virtual void SetState(const char *varName, const int value);
	virtual void SetState(const char *varName, const float value);
	virtual const char *GetStateString(const char *varName, const char *defaultString = "") const;
	virtual bool GetStateBool(const char *varName, const char *defaultString = "0") const;
	virtual int GetStateInt(const char *varName, const char *defaultString = "0") const;
	virtual float GetStateFloat(const char *varName, const char *defaultString = "0") const;
	virtual void StateChanged(int time, bool redraw);

	virtual const char *Activate(bool activate, int time);
	virtual void Trigger(int time);
	virtual void SetKeyBindingNames();

	virtual void SetCursor(float x, float y);
	virtual float CursorX() { return _cursorX; }
	virtual float CursorY() { return _cursorY; }

	size_t Size();

	Dict *GetStateDict() { return &_state; }

	const char *GetSourceFile() const { return _source.c_str(); }
	long GetTimeStamp() const { return _timeStamp; }

	Window *GetDesktop() const { return _desktop; }
	void SetBindHandler(Window *win) { _bindHandler = win; }
	bool Active() const { return _active; }
	int GetTime() const { return _time; }
	void SetTime(int _time) { _time = _time; }

	void ClearRefs() { _refs = 0; }
	void AddRef() { _refs++; }
	int GetRefs() { return _refs; }

	void RecurseSetKeyBindingNames(Window *window);
	string &GetPendingCmd() { return _pendingCmd; };
	string &GetReturnCmd() { return _returnCmd; };

private:
	bool _active;
	bool _loading;
	bool _interactive;
	bool _uniqued;

	Dict _state;
	Window *_desktop;
	Window *_bindHandler;

	string _source;
	string _activateStr;
	string _pendingCmd;
	string _returnCmd;
	long _timeStamp;

	float _cursorX;
	float _cursorY;

	int _time;
	int _refs;
};

class UserInterfaceManagerLocal : public UserInterfaceManager
{
	friend class UserInterfaceLocal;

public:
	virtual void Init();
	virtual void Shutdown();
	virtual void SetDrawingDC();
	virtual void Touch(const char *name);
	//virtual void WritePrecacheCommands(File *f);
	virtual void SetSize(float width, float height);
	//virtual void BeginLevelLoad();
	//virtual void EndLevelLoad(const char *mapName);
	//virtual void Preload(const char *mapName);
	virtual void Reload(bool all);
	virtual void ListGuis() const;
	virtual bool CheckGui(const char *path) const;
	virtual UserInterface *Alloc() const;
	virtual void DeAlloc(UserInterface *gui);
	virtual UserInterface *FindGui(const char *path, bool autoLoad = false, bool needInteractive = false, bool forceUnique = false);
	virtual UserInterface *FindDemoGui(const char *qpath);
	virtual	ListGUI *AllocListGUI() const;
	virtual void FreeListGUI(ListGUI *listgui);
	TokenParser &GetBinaryParser() { return _mapParser; }
private:
	Rectangle _screenRect;
	DeviceContext _dcOld;
	DeviceContextOptimized _dcOptimized;

	vector<UserInterfaceLocal *> _guis;
	vector<UserInterfaceLocal *> _demoGuis;

	TokenParser _mapParser;
};

extern DeviceContext *_dc;

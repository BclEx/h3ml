#ifndef __LISTGUILOCAL_H__
#define __LISTGUILOCAL_H__

class ListGUILocal : protected vector<string, TAG_OLD_UI>, public ListGUI
{
public:
	ListGUILocal() { _pGUI = nullptr; _water = 0; _stateUpdates = true; }

	// ListGUI interface
	void Config(UserInterface *gui, const char *name) { _gui = gui; _name = name; }
	void Add(int id, const string &s);
	// use the element count as index for the ids
	void Push(const string &s);
	bool Del(int id);
	void Clear();
	int Num() { return vector<string, TAG_OLD_UI>::Num(); }
	int GetSelection(char *s, int size, int sel = 0) const; // returns the id, not the list index (or -1)
	void SetSelection(int sel);
	int GetNumSelections();
	bool IsConfigured() const;
	void SetStateChanges(bool enable);
	void Shutdown();

private:
	UserInterface *_gui;
	string _name;
	int _water;
	vector<int, TAG_OLD_UI> _ids;
	bool _stateUpdates;

	void StateChanged();
};

#endif /* !__LISTGUILOCAL_H__ */

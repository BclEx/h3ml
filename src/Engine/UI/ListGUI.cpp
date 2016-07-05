#include "ListGUILocal.h"

void ListGUILocal::StateChanged()
{
	if (!_stateUpdates)
		return;
	int i;
	for (i = 0; i < Num(); i++)
		_gui->SetStateString(va("%s_item_%i", _name.c_str(), i), (*this)[i].c_str());
	for (i = Num(); i < _water; i++)
		_gui->SetStateString(va("%s_item_%i", m_name.c_str(), i), "");
	_water = Num();
	_gui->StateChanged(Sys_Milliseconds());
}

int ListGUILocal::GetNumSelections()
{
	return _gui->State().GetInt(va("%s_numsel", _name.c_str()));
}

int ListGUILocal::GetSelection(char *s, int size, int _sel) const
{
	if (s)
		s[0] = '\0';
	int sel = _gui->State().GetInt(va("%s_sel_%i", _name.c_str(), _sel), "-1");
	if (sel == -1 || sel >= _ids.Num())
		return -1;
	if (s)
		idStr::snPrintf(s, size, _gui->State().GetString(va("%s_item_%i", _name.c_str(), sel), ""));
	// don't let overflow
	if (sel >= _ids.Num())
		sel = 0;
	_gui->SetStateInt(va("%s_selid_0", _name.c_str()), _ids[sel]);
	return m_ids[sel];
}

void ListGUILocal::SetSelection(int sel)
{
	_gui->SetStateInt(va("%s_sel_0", _name.c_str()), sel);
	StateChanged();
}

void ListGUILocal::Add(int id, const string &s)
{
	int i = _ids.FindIndex(id);
	if (i == -1) {
		Append(s);
		m_ids.Append(id);
	}
	else
		(*this)[i] = s;
	StateChanged();
}

void ListGUILocal::Push(const string &s)
{
	Append(s);
	_ids.Append(_ids.Num());
	StateChanged();
}

bool ListGUILocal::Del(int id)
{
	int i = _ids.FindIndex(id);
	if (i == -1)
		return false;
	_ids.RemoveIndex(i);
	this->RemoveIndex(i);
	StateChanged();
	return true;
}

void ListGUILocal::Clear() {
	_ids.Clear();
	vector<string, TAG_OLD_UI>::Clear();
	if (_gui) // will clear all the GUI variables and will set m_water back to 0
		StateChanged();
}

bool ListGUILocal::IsConfigured() const { return (_gui != nullptr); }

void ListGUILocal::SetStateChanges(bool enable)
{
	_stateUpdates = enable;
	StateChanged();
}

void ListGUILocal::Shutdown()
{
	_gui = nullptr;
	_name.Clear();
	Clear();
}

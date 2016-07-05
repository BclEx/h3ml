#include "..\Global.h"

#include "Window.h"
#include "WinVar.h"
#include "UserInterfaceLocal.h"

WinVar::WinVar() {
	_dict = nullptr;
	_name = nullptr;
	_eval = true;
}

WinVar::~WinVar() {
	delete _name; _name = nullptr;
}

void WinVar::SetGuiInfo(Dict *dict, const char *name) {
	_dict = dict;
	SetName(name);
}

void WinVar::Init(const char *name, Window *win) {
	string key = name;
	_dict = nullptr;
	int len = key.length();
	if (len > 5 && key[0] == 'g' && key[1] == 'u' && key[2] == 'i' && key[3] == ':') {
		key = key.substr(VAR_GUIPREFIX_LEN);
		SetGuiInfo(win->GetGui()->GetStateDict(), key.c_str());
		win->AddUpdateVar(this);
	}
	else
		Set(name);
}

void MultiWinVar::Set(const char *val) {
	for (int i = 0; i < size(); i++)
		(*this)[i]->Set(val);
}

void MultiWinVar::Update() {
	for (int i = 0; i < size(); i++)
		(*this)[i]->Update();
}

void MultiWinVar::SetGuiInfo(Dict *dict) {
	for (int i = 0; i < size(); i++)
		(*this)[i]->SetGuiInfo(dict, (*this)[i]->c_str());
}


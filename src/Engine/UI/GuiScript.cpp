#include "..\Global.h"
#include "Window.h"
#include "WinVar.h"
#include "GuiScript.h"
#include "UserInterfaceLocal.h"

void Script_Set(Window *window, vector<GSWinVar> *src) {
	string key, val;
	WinStr *dest = dynamic_cast<WinStr *>((*src)[0].var);
	if (dest)
		if (!strcmp(*dest, "cmd")) {
			dest = dynamic_cast<WinStr *>((*src)[1].var);
			int parmCount = src->size();
			if (parmCount > 2) {
				val = dest->c_str();
				int i = 2;
				while (i < parmCount) {
					val += " \"";
					val += (*src)[i].var->c_str();
					val += "\"";
					i++;
				}
				window->AddCommand(val);
			}
			else
				window->AddCommand(*dest);
			return;
		}
	(*src)[0].var->Set((*src)[1].var->c_str());
	(*src)[0].var->SetEval(false);
}

void Script_SetFocus(Window *window, vector<GSWinVar> *src) {
	WinStr *parm = dynamic_cast<WinStr *>((*src)[0].var);
	if (parm) {
		drawWin_t *win = window->GetGui()->GetDesktop()->FindChildByName(*parm);
		if (win != NULL && win->win != NULL)
			window->SetFocus(win->win);
	}
}

void Script_ShowCursor(Window *window, vector<GSWinVar> *src) {
	WinStr *parm = dynamic_cast<WinStr *>((*src)[0].var);
	if (parm) {
		if (atoi(*parm))
			window->GetGui()->GetDesktop()->ClearFlag(WIN_NOCURSOR);
		else
			window->GetGui()->GetDesktop()->SetFlag(WIN_NOCURSOR);
	}
}

void Script_RunScript(Window *window, vector<GSWinVar> *src) {
	WinStr *parm = dynamic_cast<WinStr *>((*src)[0].var);
	if (parm) {
		string str = window->_cmd;
		str += " ; runScript ";
		str += parm->c_str();
		window->_cmd = str;
	}
}

void Script_LocalSound(Window *window, vector<GSWinVar> *src) {
	WinStr *parm = dynamic_cast<WinStr *>((*src)[0].var);
	if (parm)
		common->SW()->PlayShaderDirectly(*parm);
}

void Script_EvalRegs(Window *window, vector<GSWinVar> *src) {
	window->EvalRegs(-1, true);
}

void Script_EndGame(Window *window, vector<GSWinVar> *src) {
	cmdSystem->BufferCommandText(CMD_EXEC_APPEND, "disconnect\n");
}

void Script_ResetTime(Window *window, vector<GSWinVar> *src) {
	WinStr *parm = dynamic_cast<WinStr *>((*src)[0].var);
	drawWin_t *win = nullptr;
	if (parm && src->size() > 1) {
		win = window->GetGui()->GetDesktop()->FindChildByName(*parm);
		parm = dynamic_cast<WinStr *>((*src)[1].var);
	}
	if (!parm)
		return;
	if (win && win->win) {
		win->win->ResetTime(atoi(*parm));
		win->win->EvalRegs(-1, true);
	}
	else {
		window->ResetTime(atoi(*parm));
		window->EvalRegs(-1, true);
	}
}

void Script_ResetCinematics(Window *window, vector<GSWinVar> *src) {
	window->ResetCinematics();
}

void Script_Transition(Window *window, vector<GSWinVar> *src) {
	// transitions always affect rect or vec4 vars
	if (src->size() >= 4) {
		WinRectangle *rect = nullptr;
		WinVec4 *vec4 = dynamic_cast<WinVec4 *>((*src)[0].var);
		//  added float variable
		WinFloat *val = nullptr;
		if (!vec4) {
			rect = dynamic_cast<WinRectangle *>((*src)[0].var);
			//  added float variable					
			if (!rect)
				val = dynamic_cast<WinFloat *>((*src)[0].var);
		}
		WinVec4 *from = dynamic_cast<WinVec4 *>((*src)[1].var);
		WinVec4 *to = dynamic_cast<WinVec4 *>((*src)[2].var);
		WinStr *timeStr = dynamic_cast<WinStr *>((*src)[3].var);
		//  added float variable					
		if (!((vec4 || rect || val) && from && to && timeStr)) {
			// 
			common->Warning("Bad transition in gui %s in window %s\n", window->GetGui()->GetSourceFile(), window->GetName());
			return;
		}
		int time = atoi(*timeStr);
		float ac = 0.0f;
		float dc = 0.0f;
		if (src->size() > 4) {
			WinStr *acv = dynamic_cast<WinStr *>((*src)[4].var);
			WinStr *dcv = dynamic_cast<WinStr *>((*src)[5].var);
			assert(acv && dcv);
			ac = atof(*acv);
			dc = atof(*dcv);
		}
		if (vec4) {
			vec4->SetEval(false);
			window->AddTransition(vec4, *from, *to, time, ac, dc);
			//  added float variable					
		}
		else if (val) {
			val->SetEval(false);
			window->AddTransition(val, *from, *to, time, ac, dc);
		}
		else {
			rect->SetEval(false);
			window->AddTransition(rect, *from, *to, time, ac, dc);
		}
		window->StartTransition();
	}
}

typedef struct {
	const char *name;
	void(*handler)(Window *window, vector<GSWinVar> *src);
	int mMinParms;
	int mMaxParms;
} guiCommandDef_t;

guiCommandDef_t _commandList[] = {
	{ "set", Script_Set, 2, 999 },
	{ "setFocus", Script_SetFocus, 1, 1 },
	{ "endGame", Script_EndGame, 0, 0 },
	{ "resetTime", Script_ResetTime, 0, 2 },
	{ "showCursor", Script_ShowCursor, 1, 1 },
	{ "resetCinematics", Script_ResetCinematics, 0, 2 },
	{ "transition", Script_Transition, 4, 6 },
	{ "localSound", Script_LocalSound, 1, 1 },
	{ "runScript", Script_RunScript, 1, 1 },
	{ "evalRegs", Script_EvalRegs, 0, 0 }
};

int	scriptCommandCount = sizeof(_commandList) / sizeof(guiCommandDef_t);

GuiScript::GuiScript() {
	_ifList = nullptr;
	_elseList = nullptr;
	_conditionReg = -1;
	_handler = nullptr;
	//_parms.SetGranularity(2);
}

GuiScript::~GuiScript() {
	delete _ifList;
	delete _elseList;
	int c = _parms.size();
	for (int i = 0; i < c; i++)
		if (_parms[i].own)
			delete _parms[i].var;
}

bool GuiScript::Parse(TokenParser *src) {
	int i;
	// first token should be function call then a potentially variable set of parms ended with a ;
	Token token;
	if (!src->ReadToken(&token)) {
		src->Error("Unexpected end of file");
		return false;
	}
	_handler = nullptr;

	for (i = 0; i < scriptCommandCount; i++)
		if (!strcmp(token, _commandList[i].name)) {
			_handler = _commandList[i].handler;
			break;
		}
	if (!_handler)
		src->Error("Uknown script call %s", token.c_str());
	// now read parms til ;
	// all parms are read as idWinStr's but will be fixed up later 
	// to be proper types
	while (1) {
		if (!src->ReadToken(&token)) {
			src->Error("Unexpected end of file");
			return false;
		}
		if (!strcmp(token, ";"))
			break;
		if (!strcmp(token, "}")) {
			src->UnreadToken(&token);
			break;
		}

		WinStr *str = new WinStr();
		*str = token;
		GSWinVar wv;
		wv.own = true;
		wv.var = str;
		_parms.push_back(wv);
	}

	//  verify min/max params
	if (handler && (_parms.size() < _commandList[i].mMinParms || _parms.size() > _commandList[i].mMaxParms))
		src->Error("incorrect number of parameters for script %s", _commandList[i].name);
	return true;
}

void GuiScriptList::Execute(Window *win) {
	int c = _list.size();
	for (int i = 0; i < c; i++) {
		GuiScript *gs = _list[i];
		assert(gs);
		if (gs->_conditionReg >= 0)
			if (win->HasOps()) {
				float f = win->EvalRegs(gs->_conditionReg);
				if (f) {
					if (gs->_ifList)
						win->RunScriptList(gs->_ifList);
				}
				else if (gs->_elseList)
					win->RunScriptList(gs->_elseList);
			}
		gs->Execute(win);
	}
}

void GuiScript::FixupParms(Window *win) {
	if (_handler == &Script_Set) {
		bool precacheBackground = false;
		bool precacheSounds = false;
		WinStr *str = dynamic_cast<WinStr *>(_parms[0].var);
		assert(str);
		WinVar *dest = win->GetWinVarByName(*str, true);
		if (dest) {
			delete _parms[0].var;
			_parms[0].var = dest;
			_parms[0].own = false;

			if (dynamic_cast<WinBackground *>(dest) != nullptr)
				precacheBackground = true;
		}
		else if (!strcmp(str->c_str(), "cmd"))
			precacheSounds = true;
		int parmCount = _parms.size();
		for (int i = 1; i < parmCount; i++) {
			WinStr *str = dynamic_cast<WinStr *>(_parms[i].var);
			if (!strncmp(*str, "gui::", 5)) {
				//  always use a string here, no point using a float if it is one
				//  FIXME: This creates duplicate variables, while not technically a problem since they
				//  are all bound to the same guiDict, it does consume extra memory and is generally a bad thing
				WinStr *defvar = new WinStr();
				defvar->Init(*str, win);
				win->AddDefinedVar(defvar);
				delete _parms[i].var;
				_parms[i].var = defvar;
				_parms[i].own = false;

				//dest = win->GetWinVarByName(*str, true);
				//if (dest) {
				//	delete parms[i].var;
				//	parms[i].var = dest;
				//	parms[i].own = false;
				//}
			}
			else if ((*str[0]) == '$') {
				//  dont include the $ when asking for variable
				dest = win->GetGui()->GetDesktop()->GetWinVarByName((const char*)(*str) + 1, true);
				if (dest) {
					delete _parms[i].var;
					_parms[i].var = dest;
					_parms[i].own = false;
				}
			}
			else if (!strncmp(str->c_str(), STRTABLE_ID, STRTABLE_ID_LENGTH))
				str->Set(Localization::GetString(str->c_str()));
			else if (precacheBackground) {
				const Material *mat = g_declManager->FindMaterial(str->c_str());
				_mat->SetSort(SS_GUI);
			}
			else if (precacheSounds) {
				// Search for "play <...>"
				Token token;
				Parser parser(LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT);
				parser.LoadMemory(str->c_str(), str->Length(), "command");

				while (parser.ReadToken(&token))
					if (!token.Icmp("play"))
						if (parser.ReadToken(&token) && token != "")
							g_declManager->FindSound(token.c_str());
			}
		}
	}
	else if (handler == &Script_Transition) {
		if (_parms.size() < 4)
			common->Warning("Window %s in gui %s has a bad transition definition", win->GetName(), win->GetGui()->GetSourceFile());
		WinStr *str = dynamic_cast<WinStr *>(_parms[0].var);
		assert(str);

		// 
		drawWin_t *destowner;
		WinVar *dest = win->GetWinVarByName(*str, true, &destowner);
		if (dest) {
			delete _parms[0].var;
			_parms[0].var = dest;
			_parms[0].own = false;
		}
		else
			common->Warning("Window %s in gui %s: a transition does not have a valid destination var %s", win->GetName(), win->GetGui()->GetSourceFile(), str->c_str());

		//  support variables as parameters		
		int c;
		for (c = 1; c < 3; c++) {
			str = dynamic_cast<WinStr *>(_parms[c].var);

			WinVec4 *v4 = new WinVec4;
			_parms[c].var = v4;
			_parms[c].own = true;

			drawWin_t *owner = nullptr;
			if ((*str[0]) == '$')
				dest = win->GetWinVarByName((const char*)(*str) + 1, true, &owner);
			else
				dest = nullptr;
			if (dest) {
				Window *ownerparent;
				Window *destparent;
				if (owner) {
					ownerparent = (owner->simp ? owner->simp->GetParent() : owner->win->GetParent());
					destparent = (destowner->simp ? destowner->simp->GetParent() : destowner->win->GetParent());

					// If its the rectangle they are referencing then adjust it 
					if (ownerparent && destparent && (dest == (owner->simp ? owner->simp->GetWinVarByName("rect") : owner->win->GetWinVarByName("rect")))) {
						Rectangle rect = *(dynamic_cast<WinRectangle *>(dest));
						ownerparent->ClientToScreen(&rect);
						destparent->ScreenToClient(&rect);
						*v4 = rect.ToVec4();
					}
					else
						v4->Set(dest->c_str());
				}
				else
					v4->Set(dest->c_str());
			}
			else
				v4->Set(*str);
			delete str;
		}
		// 
	}
	else if (_handler == &Script_LocalSound) {
		WinStr *str = dynamic_cast<WinStr *>(_parms[0].var);
		if (str)
			g_declManager->FindSound(str->c_str());
	}
	else {
		int c = _parms.size();
		for (int i = 0; i < c; i++)
			_parms[i].var->Init(_parms[i].var->c_str(), win);
	}
}

void GuiScriptList::FixupParms(Window *win) {
	int c = _list.size();
	for (int i = 0; i < c; i++) {
		GuiScript *gs = _list[i];
		gs->FixupParms(win);
		if (gs->_ifList)
			gs->_ifList->FixupParms(win);
		if (gs->_elseList)
			gs->_elseList->FixupParms(win);
	}
}

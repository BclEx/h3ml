#include "RegExp.h"
#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"

int Register::REGCOUNT[NUMTYPES] = { 4, 1, 1, 1, 0, 2, 3, 4 };

void Register::SetToRegs(float *registers)
{
	int i;
	idVec4 v;
	idVec2 v2;
	idVec3 v3;
	idRectangle rect;

	if (!enabled || var == nullptr || (var && (var->GetDict() || !var->GetEval())))
		return;
	switch (type) {
	case VEC4: {
		v = *static_cast<WinVec4 *>(var);
		break;
	}
	case RECTANGLE: {
		rect = *static_cast<WinRectangle *>(var);
		v = rect.ToVec4();
		break;
	}
	case VEC2: {
		v2 = *static_cast<WinVec2 *>(var);
		v[0] = v2[0];
		v[1] = v2[1];
		break;
	}
	case VEC3: {
		v3 = *static_cast<WinVec3 *>(var);
		v[0] = v3[0];
		v[1] = v3[1];
		v[2] = v3[2];
		break;
	}
	case FLOAT: {
		v[0] = *static_cast<WinFloat *>(var);
		break;
	}
	case INT: {
		v[0] = *static_cast<WinInt *>(var);
		break;
	}
	case BOOL: {
		v[0] = *static_cast<WinBool *>(var);
		break;
	}
	default: {
		common->FatalError("idRegister::SetToRegs: bad reg type");
		break;
	}
	}
	for (i = 0; i < regCount; i++)
		registers[regs[i]] = v[i];
}

/*
=================
idRegister::GetFromRegs
=================
*/
void Register::GetFromRegs(float *registers)
{
	Vec4 v;
	Rectangle rect;

	if (!enabled || var == nullptr || (var && (var->GetDict() || !var->GetEval())))
		return;
	for (int i = 0; i < regCount; i++)
		v[i] = registers[regs[i]];
	switch (type) {
	case VEC4: {
		*dynamic_cast<WinVec4 *>(var) = v;
		break;
	}
	case RECTANGLE: {
		rect.x = v.x;
		rect.y = v.y;
		rect.w = v.z;
		rect.h = v.w;
		*static_cast<WinRectangle *>(var) = rect;
		break;
	}
	case VEC2: {
		*static_cast<WinVec2 *>(var) = v.ToVec2();
		break;
	}
	case VEC3: {
		*static_cast<WinVec3 *>(var) = v.ToVec3();
		break;
	}
	case FLOAT: {
		*static_cast<WinFloat *>(var) = v[0];
		break;
	}
	case INT: {
		*static_cast<WinInt *>(var) = v[0];
		break;
	}
	case BOOL: {
		*static_cast<WinBool *>(var) = (v[0] != 0.0f);
		break;
	}
	default: {
		common->FatalError("idRegister::GetFromRegs: bad reg type");
		break;
	}
	}
}

//void idRegisterList::AddReg(const char *name, int type, idVec4 data, idWindow *win, idWinVar *var) {
//	if (FindReg(name) == NULL) {
//		assert(type >= 0 && type < idRegister::NUMTYPES);
//		int numRegs = idRegister::REGCOUNT[type];
//		idRegister *reg = new (TAG_OLD_UI) idRegister(name, type);
//		reg->var = var;
//		for (int i = 0; i < numRegs; i++) {
//			reg->regs[i] = win->ExpressionConstant(data[i]);
//		}
//		int hash = regHash.GenerateKey(name, false);
//		regHash.Add(hash, regs.Append(reg));
//	}
//}
//
//void idRegisterList::AddReg(const char *name, int type, idTokenParser *src, idWindow *win, idWinVar *var) {
//	idRegister* reg;
//
//	reg = FindReg(name);
//
//	if (reg == NULL) {
//		assert(type >= 0 && type < idRegister::NUMTYPES);
//		int numRegs = idRegister::REGCOUNT[type];
//		reg = new (TAG_OLD_UI) idRegister(name, type);
//		reg->var = var;
//		if (type == idRegister::STRING) {
//			idToken tok;
//			if (src->ReadToken(&tok)) {
//				tok = idLocalization::GetString(tok);
//				var->Init(tok, win);
//			}
//		}
//		else {
//			for (int i = 0; i < numRegs; i++) {
//				reg->regs[i] = win->ParseExpression(src, NULL);
//				if (i < numRegs - 1) {
//					src->ExpectTokenString(",");
//				}
//			}
//		}
//		int hash = regHash.GenerateKey(name, false);
//		regHash.Add(hash, regs.Append(reg));
//	}
//	else {
//		int numRegs = idRegister::REGCOUNT[type];
//		reg->var = var;
//		if (type == idRegister::STRING) {
//			idToken tok;
//			if (src->ReadToken(&tok)) {
//				var->Init(tok, win);
//			}
//		}
//		else {
//			for (int i = 0; i < numRegs; i++) {
//				reg->regs[i] = win->ParseExpression(src, NULL);
//				if (i < numRegs - 1) {
//					src->ExpectTokenString(",");
//				}
//			}
//		}
//	}
//}

void RegisterList::GetFromRegs(float *registers)
{
	for (int i = 0; i < _regs.Num(); i++)
		_regs[i]->GetFromRegs(registers);
}

void RegisterList::SetToRegs(float *registers)
{
	for (int i = 0; i < _regs.Num(); i++)
		_regs[i]->SetToRegs(registers);
}

Register *RegisterList::FindReg(const char *name)
{
	int hash = _regHash.GenerateKey(name, false);
	for (int i = _regHash.First(hash); i != -1; i = _regHash.Next(i))
		if (_regs[i]->name.Icmp(name) == 0)
			return _regs[i];
	return nullptr;
}

void RegisterList::Reset()
{
	_regs.DeleteContents(true);
	_regHash.Clear();
}
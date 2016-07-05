#pragma once
#ifndef __PRECOMPILED_H__
#define __PRECOMPILED_H__

#include <string>
#include <map>
#include <vector>
#include <glm.hpp>
char *va(const char *fmt, ...);
using namespace std;
using namespace glm;
// id lib
#include "lib/Lib.h"

// decls
#include "Framework/TokenParser.h"
#include "Framework/DeclManager.h"

// We have expression parsing and evaluation code in multiple places:
// materials, sound shaders, and guis. We should unify them.
const int MAX_EXPRESSION_OPS = 4096;
const int MAX_EXPRESSION_REGISTERS = 4096;

// renderer
//#include "Renderer/OpenGL/qgl.h"
#include "Renderer/Material.h"
//#include "Renderer/BufferObject.h"
//#include "Renderer/VertexCache.h"
//#include "Renderer/Model.h"
//#include "Renderer/ModelManager.h"
//#include "Renderer/RenderSystem.h"
//#include "Renderer/RenderWorld.h"

// user interfaces
#include "UI/ListGUI.h"
#include "UI/UserInterface.h"

#undef min
#undef max
#include <algorithm>	// for min / max / swap

#endif /* !__PRECOMPILED_H__ */

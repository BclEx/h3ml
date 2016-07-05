struct guiModelSurface_t {
	const Material *material;
	uint64 glState;
	int firstIndex;
	int numIndexes;
};

class GuiModel {
public:
	GuiModel();

	void Clear();

	// allocates memory for verts and indexes in frame-temporary buffer memory
	void BeginFrame();

	void EmitToCurrentView(float modelMatrix[16], bool depthHack);
	void EmitFullScreen();

	// the returned pointer will be in write-combined memory, so only make contiguous 32 bit writes and never read from it.
	DrawVert *AllocTris(int numVerts, const triIndex_t *indexes, int numIndexes, const Material *material, const uint64 glState);

	//---------------------------
private:
	void AdvanceSurf();
	void EmitSurfaces(float modelMatrix[16], float modelViewMatrix[16], bool depthHack, bool allowFullScreenStereoDepth, bool linkAsEntity);

	guiModelSurface_t *_surf;

	float _shaderParms[MAX_ENTITY_SHADER_PARMS];

	// if we exceed these limits we stop rendering GUI surfaces
	static const int MAX_INDEXES = (20000 * 6);
	static const int MAX_VERTS = (20000 * 4);

	vertCacheHandle_t _vertexBlock;
	vertCacheHandle_t _indexBlock;
	DrawVert *_vertexPointer;
	triIndex_t *_indexPointer;

	int _numVerts;
	int _numIndexes;

	vector<guiModelSurface_t> _surfaces;
};


#define API extern "C" __declspec(dllexport)
#define DEM_API_VER_MAJOR	0
#define DEM_API_VER_MINOR	1
#define DEM_API_VER_STEP	17
#define DEM_API_VERSION ((DEM_API_VER_MAJOR << 16) + (DEM_API_VER_MINOR << 8) + DEM_API_VER_STEP)
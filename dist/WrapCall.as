#ifndef IG_HPI_WRAP_CALL_AS
#define IG_HPI_WRAP_CALL_AS
#ifdef _DEBUG

#ifdef __hsp64__
 #regcmd "hsp3hpi_init_wrapcall", "hsp3debug_64.dll"
#else
#ifdef __hsp3utf__
 #regcmd "_hsp3hpi_init_wrapcall@4", "hsp3debug_u8.dll"
#else
 #regcmd "_hsp3hpi_init_wrapcall@4", "hsp3debug.dll"
#endif
#endif ;defined(__hsp64__)

#endif ;defined(_DEBUG)
#endif

#ifndef AX_VERSION_COMPARE_H
#define AX_VERSION_COMPARE_H

#ifdef __cplusplus
extern "C"
{
#endif
    // compare compile sdk version and board sdk version
    // return 0 if equal, -1 if not
    int ax_version_check();
#ifdef __cplusplus
}
#endif
#endif
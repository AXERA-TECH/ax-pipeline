#ifndef WARPAFFINE_H
#define WARPAFFINE_H
#ifdef __cplusplus
extern "C"
{
#endif
    void get_rotation_matrix(float angle, float scale, float dx, float dy, float *tm);
    void get_affine_transform(const float *points_from, const float *points_to, int num_point, float *tm);
    void invert_affine_transform(const float *tm, float *tm_inv);
#ifdef __cplusplus
}
#endif
#endif // WARPAFFINE_H
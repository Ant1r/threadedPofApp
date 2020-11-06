extern void sharetable_setup(void);
extern void hextab_setup(void);

#ifdef __cplusplus
extern "C"
{
#endif

void externals_setup()
{
	hextab_setup();
	sharetable_setup();
}

#ifdef __cplusplus
}
#endif


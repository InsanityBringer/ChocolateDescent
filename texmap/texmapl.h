
/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

 //	Local include file for texture map library.

extern	int prevmod(int val, int modulus);
extern	int succmod(int val, int modulus);
extern	void texture_map_flat(g3ds_tmap* t, int color);

extern float compute_dx_dy(g3ds_tmap* t, int top_vertex, int bottom_vertex, float recip_dy);
extern void compute_y_bounds(g3ds_tmap* t, int* vlt, int* vlb, int* vrt, int* vrb, int* bottom_y_ind);

extern int	fx_y, fx_xleft, fx_xright, per2_flag;
extern unsigned char tmap_flat_color;
extern unsigned char* pixptr;

/*
extern fix compute_dx_dy_lin(g3ds_tmap* t, int vlt, int vlb, fix recip_dy);
extern fix compute_dx_dy_lin(g3ds_tmap* t, int vrt, int vrb, fix recip_dy);*/
extern float compute_du_dy_lin(g3ds_tmap* t, int vlt, int vlb, float recip_dy);
extern float compute_du_dy_lin(g3ds_tmap* t, int vrt, int vrb, float recip_dy);
extern float compute_dv_dy_lin(g3ds_tmap* t, int vlt, int vlb, float recip_dy);
extern float compute_dv_dy_lin(g3ds_tmap* t, int vrt, int vrb, float recip_dy);


// Interface variables to assembler code
extern	float	fx_u, fx_v, fx_z, fx_du_dx, fx_dv_dx, fx_dz_dx;
extern	float	fx_dl_dx, fx_l;
extern	int	fx_r, fx_g, fx_b, fx_dr_dx, fx_dg_dx, fx_db_dx;
extern	unsigned char* pixptr;

extern	int	bytes_per_row;
//extern	int	write_buffer;
extern uint8_t* write_buffer;
extern	int  	window_left;
extern	int	window_right;
extern	int	window_top;
extern	int	window_bottom;
extern	int  	window_width;
extern	int  	window_height;
extern	int	scan_doubling_flag;
extern	int	linear_if_far_flag;
extern	int	dither_intensity_lighting;
extern	int	Interlacing_on;

extern	short	_pixel_data_selector;

extern uint8_t* tmap_flat_cthru_table;
extern uint8_t tmap_flat_color;
extern uint8_t tmap_flat_shade_value;


extern float fix_recip[];

extern void init_interface_vars_to_assembler(void);
extern int prevmod(int val, int modulus);

#define FIX_RECIP_TABLE_SIZE	321


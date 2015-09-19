/*
*	This file is part of KNLMeansCL,
*	Copyright(C) 2014-2015  Khanattila.
*
*	KNLMeansCL is free software: you can redistribute it and/or modify
*	it under the terms of the GNU Lesser General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	KNLMeansCL is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*	GNU Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public License
*	along with KNLMeansCL. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __KERNEL_H__
#define __KERNEL_H__

//////////////////////////////////////////
// OpenCL
static const cl_uint H_BLOCK_X = 32, H_BLOCK_Y = 4, V_BLOCK_X = 32, V_BLOCK_Y = 4;
static const char* source_code =
"																												  \n" \
"#define wRED    0.6664827524f        	                   														  \n" \
"#define wGREEN	 1.2866580779f        	        																  \n" \
"#define wBLUE	 1.0468591696f        						            										  \n" \
"#define wMSB 	 256.0f / 257.0f        																		  \n" \
"#define wLSB    1.0f / 257.0f      																			  \n" \
"																												  \n" \
"enum {                                                                                                           \n" \
"    COLOR_GRAY   = 1 << 0, COLOR_YUV    = 1 << 1, COLOR_RGB    = 1 << 2,                                         \n" \
"    COLOR_MASK   = 1 << 0               | 1 << 1               | 1 << 2,                                         \n" \
"    CLIP_REGULAR = 1 << 3, CLIP_STACKED = 1 << 4,                                                                \n" \
"    CLIP_MASK    = 1 << 3               | 1 << 4,                                                                \n" \
"    EXTRA_NCLIP  = 1 << 5, EXTRA_YCLIP  = 1 << 6,                                                                \n" \
"    EXTRA_MASK   = 1 << 5               | 1 << 6                                                                 \n" \
"};                                                                                                               \n" \
"																												  \n" \
"float   norm(uint u)      { return native_divide((float) (u << NLMK_BIT_SHIFT), (float) USHRT_MAX); }            \n" \
"ushort  denorm(float f)   { return convert_ushort_sat(f * (float) USHRT_MAX) >> NLMK_BIT_SHIFT; }                \n" \
"ushort4 denorm4(float4 f) { return convert_ushort4_sat(f * (float4) USHRT_MAX) >> (ushort4) NLMK_BIT_SHIFT; }    \n" \
"																												  \n" \
"__kernel																										  \n" \
"void NLM_init(__global void* U2, __global float* M, const int2 dim) {											  \n" \
"																												  \n" \
"	const int x = get_global_id(0);																				  \n" \
"	const int y = get_global_id(1);																				  \n" \
"	if(x >= dim.x || y >= dim.y) return;																		  \n" \
"																												  \n" \
"	const int gidx = mad24(y, dim.x, x);																		  \n" \
"																												  \n" \
"	if (NLMK_TCOLOR & COLOR_GRAY) {	                    														  \n" \
"		__global float2* U2c = (__global float2*) U2;															  \n" \
"		U2c[gidx] = (float2) 0.0f;																				  \n" \
"	} else {			                        										                          \n" \
"		__global float4* U2c = (__global float4*) U2;															  \n" \
"		U2c[gidx] = (float4) 0.0f;																				  \n" \
"	}																											  \n" \
"	M[gidx] = FLT_EPSILON;																						  \n" \
"}																												  \n" \
"																												  \n" \
"__kernel																										  \n" \
"void NLM_dist(__read_only image2d_t U1, __read_only image2d_t U1_pq, __write_only image2d_t U4, const int2 dim,  \n" \
"const int2 q) {																								  \n" \
"																												  \n" \
"	const int x = get_global_id(0);																				  \n" \
"	const int y = get_global_id(1);																				  \n" \
"	if(x >= dim.x || y >= dim.y) return;																		  \n" \
"																												  \n" \
"	const sampler_t smp = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;		        	  \n" \
"	const int2 coord2 = (int2) (x, y);		                            										  \n" \
"																												  \n" \
"	if (NLMK_TCOLOR & COLOR_GRAY) {	                    														  \n" \
"		const float u1 = read_imagef(U1, smp, coord2).x;			     							              \n" \
"		const float u1_pq = read_imagef(U1_pq, smp, coord2 + q).x;									              \n" \
"		const float val = 3.0f * (u1 - u1_pq) * (u1 - u1_pq);                                                     \n" \
"		write_imagef(U4, coord2, (float4) val);								    								  \n" \
"	} else if (NLMK_TCOLOR & COLOR_YUV) {																		  \n" \
"		const float4 u1 = read_imagef(U1, smp, coord2);			     	        							      \n" \
"		const float4 u1_pq = read_imagef(U1_pq, smp, coord2 + q);	        									  \n" \
"		const float4 dist = (u1 - u1_pq) * (u1 - u1_pq);														  \n" \
"		const float val = dist.x + dist.y + dist.z;                                                               \n" \
"		write_imagef(U4, coord2, (float4) val);								    								  \n" \
"	} else if (NLMK_TCOLOR & COLOR_RGB) {																		  \n" \
"		const float4 u1 = read_imagef(U1, smp, coord2);			     		        						      \n" \
"		const float4 u1_pq = read_imagef(U1_pq, smp, coord2 + q);		        								  \n" \
"		const float4 wgh = (float4) (wRED, wGREEN, wBLUE, 0.0f);												  \n" \
"		const float4 dist = wgh * (u1 - u1_pq) * (u1 - u1_pq);												      \n" \
"		const float val = dist.x + dist.y + dist.z;                                                               \n" \
"		write_imagef(U4, coord2, (float4) val);								    								  \n" \
"	}																											  \n" \
"}																												  \n" \
"																												  \n" \
"__kernel __attribute__((reqd_work_group_size(H_BLOCK_X, H_BLOCK_Y, 1)))										  \n" \
"void NLM_horiz(__read_only image2d_t U4_in, __write_only image2d_t U4_out,	const int2 dim) {					  \n" \
"																												  \n" \
"	__local float buffer[H_BLOCK_Y][3*H_BLOCK_X];																  \n" \
"	const int x = get_global_id(0);																				  \n" \
"	const int y = get_global_id(1);																				  \n" \
"	const int lx = get_local_id(0);																				  \n" \
"	const int ly = get_local_id(1);																				  \n" \
"																												  \n" \
"	const sampler_t smp = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;		        	  \n" \
"	const int2 coord2 = (int2) (x, y);		                            										  \n" \
"																												  \n" \
"	buffer[ly][lx + H_BLOCK_X]	 = read_imagef(U4_in, smp, coord2).x;                      						  \n" \
"	buffer[ly][lx]		         = read_imagef(U4_in, smp, coord2 - (int2) (H_BLOCK_X, 0)).x;					  \n" \
"	buffer[ly][lx + 2*H_BLOCK_X] = read_imagef(U4_in, smp, coord2 + (int2) (H_BLOCK_X, 0)).x;					  \n" \
"	barrier(CLK_LOCAL_MEM_FENCE);																				  \n" \
"																												  \n" \
"	if(x >= dim.x || y >= dim.y) return;																		  \n" \
"	float sum = 0.0f;																							  \n" \
"	for(int i = -NLMK_S; i <= NLMK_S; i++)																		  \n" \
"		sum += buffer[ly][lx + H_BLOCK_X + i];																	  \n" \
"	write_imagef(U4_out, coord2, (float4) sum);																	  \n" \
"}																												  \n" \
"																												  \n" \
"__kernel __attribute__((reqd_work_group_size(V_BLOCK_X, V_BLOCK_Y, 1)))										  \n" \
"void NLM_vert(__read_only image2d_t U4_in, __write_only image2d_t U4_out,	const int2 dim) {					  \n" \
"																												  \n" \
"	__local float buffer[3*V_BLOCK_Y][V_BLOCK_X];																  \n" \
"	const int x = get_global_id(0);																				  \n" \
"	const int y = get_global_id(1);																				  \n" \
"	const int lx = get_local_id(0);																				  \n" \
"	const int ly = get_local_id(1);																				  \n" \
"																												  \n" \
"	const sampler_t smp = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;		        	  \n" \
"	const int2 coord2 = (int2) (x, y);		                            										  \n" \
"																												  \n" \
"	buffer[ly + V_BLOCK_Y][lx]	 = read_imagef(U4_in, smp, coord2).x;											  \n" \
"	buffer[ly][lx]		         = read_imagef(U4_in, smp, coord2 - (int2) (0, V_BLOCK_Y)).x;					  \n" \
"	buffer[ly + 2*V_BLOCK_Y][lx] = read_imagef(U4_in, smp, coord2 + (int2) (0, V_BLOCK_Y)).x;    				  \n" \
"	barrier(CLK_LOCAL_MEM_FENCE);																				  \n" \
"																												  \n" \
"	if(x >= dim.x || y >= dim.y) return;																		  \n" \
"	float sum = 0.0f;																							  \n" \
"	for(int j = -NLMK_S; j <= NLMK_S; j++)																		  \n" \
"		sum += buffer[ly + V_BLOCK_Y + j][lx];																	  \n" \
"																												  \n" \
"	if(NLMK_WMODE == 0) {																						  \n" \
"		const float val = native_recip(1.0f + sum * NLMK_H2_INV_NORM); 											  \n" \
"	    write_imagef(U4_out, coord2, (float4) val);                                                               \n" \
"	} else if (NLMK_WMODE == 1) {																				  \n" \
"		const float val = native_exp(- sum * NLMK_H2_INV_NORM);													  \n" \
"	    write_imagef(U4_out, coord2, (float4) val);                                                               \n" \
"	} else if (NLMK_WMODE == 2) {																				  \n" \
"		const float val = pown(fdim(1.0f, sum * NLMK_H2_INV_NORM), 2); 											  \n" \
"	    write_imagef(U4_out, coord2, (float4) val);                                                               \n" \
"	}	                                                                										  \n" \
"}																												  \n" \
"																												  \n" \
"__kernel																										  \n" \
"void NLM_accu(__read_only image2d_t U1, __global void* U2, __read_only image2d_t U4, __global float* M,		  \n" \
"const int2 dim, const int2 q) {																				  \n" \
"																												  \n" \
"	const int x = get_global_id(0);																				  \n" \
"	const int y = get_global_id(1);																				  \n" \
"	if(x >= dim.x || y >= dim.y) return;																		  \n" \
"																												  \n" \
"	const sampler_t smp = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;	        		  \n" \
"	const int2 coord2 = (int2) (x, y);		                            										  \n" \
"	const int gidx = mad24(y, dim.x, x);																		  \n" \
"																												  \n" \
"	const float u4 = read_imagef(U4, smp, coord2).x;  															  \n" \
"	const float u4_mq = read_imagef(U4, smp, coord2 - q).x;														  \n" \
"	M[gidx] = fmax(M[gidx], fmax(u4, u4_mq));																	  \n" \
"																												  \n" \
"	if (NLMK_TCOLOR & COLOR_GRAY) {																				  \n" \
"		__global float2* U2c = (__global float2*) U2;													          \n" \
"		const float u1_pq = read_imagef(U1, smp, coord2 + q).x;										              \n" \
"		const float u1_mq = read_imagef(U1, smp, coord2 - q).x;									                  \n" \
"		float2 accu;   							                                                                  \n" \
"		accu.x = NLMK_TEMPORAL ? (u4 * u1_pq) : (u4 * u1_pq) + (u4_mq * u1_mq);			    		        	  \n" \
"		accu.y = NLMK_TEMPORAL ? (u4) : (u4 + u4_mq);			    								        	  \n" \
"		U2c[gidx] += accu;																						  \n" \
"	} else if (NLMK_TCOLOR & COLOR_YUV) {								                                          \n" \
"		__global float4* U2c = (__global float4*) U2;															  \n" \
"		const float4 u1_pq = read_imagef(U1, smp, coord2 + q);											          \n" \
"		const float4 u1_mq = read_imagef(U1, smp, coord2 - q);											          \n" \
"		float4 accu   = NLMK_TEMPORAL ? (u4 * u1_pq) : (u4 * u1_pq) + (u4_mq * u1_mq);							  \n" \
"		       accu.w = NLMK_TEMPORAL ? (u4) : (u4 + u4_mq);													  \n" \
"		U2c[gidx] += accu;																						  \n" \
"	} else if (NLMK_TCOLOR & COLOR_RGB) {								                                          \n" \
"		__global float4* U2c = (__global float4*) U2;															  \n" \
"		const float4 u1_pq = read_imagef(U1, smp, coord2 + q);								        			  \n" \
"		const float4 u1_mq = read_imagef(U1, smp, coord2 - q);									        		  \n" \
"		float4 accu   = NLMK_TEMPORAL ? (u4 * u1_pq) : (u4 * u1_pq) + (u4_mq * u1_mq);							  \n" \
"		       accu.w = NLMK_TEMPORAL ? (u4) : (u4 + u4_mq);													  \n" \
"		U2c[gidx] += accu;																						  \n" \
"	}																											  \n" \
"}																												  \n" \
"																												  \n" \
"__kernel																										  \n" \
"void NLM_finish(__read_only image2d_t U1_in, __write_only image2d_t U1_out, __global void* U2,					  \n" \
"__global float* M, const int2 dim) {																			  \n" \
"																												  \n" \
"	const int x = get_global_id(0);																				  \n" \
"	const int y = get_global_id(1);																				  \n" \
"	if(x >= dim.x || y >= dim.y) return;																		  \n" \
"																												  \n" \
"	const sampler_t smp = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;	        		  \n" \
"	const int2 coord2 = (int2) (x, y);		                            										  \n" \
"	const int gidx = mad24(y, dim.x, x);            															  \n" \
"       																										  \n" \
"	if (NLMK_TCOLOR & COLOR_GRAY) {																				  \n" \
"		__global float2* U2c = (__global float2*) U2;														      \n" \
"		const float u1 = read_imagef(U1_in, smp, coord2).x;		        									      \n" \
"		const float num = U2c[gidx].x + M[gidx] * u1;	    												      \n" \
"		const float den = U2c[gidx].y + M[gidx];															      \n" \
"		const float val = native_divide(num, den);															      \n" \
"		write_imagef(U1_out, coord2, (float4) val);		        					                    	      \n" \
"	} else if (NLMK_TCOLOR & COLOR_YUV) {												                          \n" \
"		__global float4* U2c = (__global float4*) U2;															  \n" \
"		const float4 u1 = read_imagef(U1_in, smp, coord2);		        										  \n" \
"		const float4 num = U2c[gidx] + (float4) M[gidx] * u1;					            					  \n" \
"		const float  den = U2c[gidx].w + M[gidx];			            										  \n" \
"		const float4 val = native_divide(num, (float4) den);									                  \n" \
"		write_imagef(U1_out, coord2, val);			        													  \n" \
"	} else if (NLMK_TCOLOR & COLOR_RGB) {												                          \n" \
"		__global float4* U2c = (__global float4*) U2;															  \n" \
"		const float4 u1 = read_imagef(U1_in, smp, coord2);		        										  \n" \
"		const float4 num = U2c[gidx] + (float4) M[gidx] * u1;					            					  \n" \
"		const float  den = U2c[gidx].w + M[gidx];			            										  \n" \
"		const float4 val = native_divide(num, (float4) den);									                  \n" \
"		write_imagef(U1_out, coord2, val);					        											  \n" \
"	}       																									  \n" \
"}																												  \n" \
"																												  \n" \
"__kernel																										  \n" \
"void NLM_pack(__read_only image2d_t R, __read_only image2d_t G, __read_only image2d_t B,   					  \n" \
"__write_only image2d_t U1, const int2 dim) { 																	  \n" \
"																												  \n" \
"	const int x = get_global_id(0);																				  \n" \
"	const int y = get_global_id(1);																				  \n" \
"	if(x >= dim.x || y >= dim.y) return;																		  \n" \
"																												  \n" \
"	const sampler_t smp = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;		        	  \n" \
"	const int2 coord2 = (int2) (x, y);		                            										  \n" \
"																												  \n" \
"	if (NLMK_TCOLOR & COLOR_GRAY) {																				  \n" \
"	    if (NLMK_LSB == 1) {																			          \n" \
"	        const int2 coord2_lsb = (int2) (x, y + dim.y);	               										  \n" \
"	        const float r_msb = read_imagef(R, smp, coord2).x;         											  \n" \
"	        const float r_lsb = read_imagef(R, smp, coord2_lsb).x;      										  \n" \
"	        const float r = wMSB * r_msb + wLSB * r_lsb;                 										  \n" \
"	        write_imagef(U1, coord2, (float4) r);	            									              \n" \
"	    } else if (NLMK_BIT_SHIFT != 0) {																          \n" \
"	        const float r = norm(read_imageui(R, smp, coord2).x);      											  \n" \
"	        write_imagef(U1, coord2, (float4) r);	            									              \n" \
"	    }																										  \n" \
"	} else if (NLMK_TCOLOR & COLOR_YUV) {												                          \n" \
"	    if (NLMK_BIT_SHIFT == 0) {																			      \n" \
"	        if (NLMK_LSB == 0) {																			      \n" \
"	            const float r = read_imagef(R, smp, coord2).x;          										  \n" \
"	            const float g = read_imagef(G, smp, coord2).x;													  \n" \
"	            const float b = read_imagef(B, smp, coord2).x;													  \n" \
"	            write_imagef(U1, coord2, (float4) (r, g, b, 1.0f));										          \n" \
"	        } else if (NLMK_LSB == 1) {																			  \n" \
"	            const int2 coord2_lsb = (int2) (x, y + dim.y);	           										  \n" \
"	            const float r_msb = read_imagef(R, smp, coord2).x;          									  \n" \
"	            const float g_msb = read_imagef(G, smp, coord2).x;												  \n" \
"	            const float b_msb = read_imagef(B, smp, coord2).x;												  \n" \
"	            const float r_lsb = read_imagef(R, smp, coord2_lsb).x;          								  \n" \
"	            const float g_lsb = read_imagef(G, smp, coord2_lsb).x;											  \n" \
"	            const float b_lsb = read_imagef(B, smp, coord2_lsb).x;											  \n" \
"	            const float r = wMSB * r_msb + wLSB * r_lsb;                 									  \n" \
"	            const float g = wMSB * g_msb + wLSB * g_lsb;                 									  \n" \
"	            const float b = wMSB * b_msb + wLSB * b_lsb;                 									  \n" \
"	            write_imagef(U1, coord2, (float4) (r, g, b, 1.0f));										          \n" \
"	        }																									  \n" \
"	    } else if (NLMK_BIT_SHIFT != 0) {																		  \n" \
"	        const float r = norm(read_imageui(R, smp, coord2).x);      											  \n" \
"	        const float g = norm(read_imageui(G, smp, coord2).x);												  \n" \
"	        const float b = norm(read_imageui(B, smp, coord2).x);												  \n" \
"	        write_imagef(U1, coord2, (float4) (r, g, b, 1.0f));										              \n" \
"	    }																										  \n" \
"	} else if (NLMK_TCOLOR & COLOR_RGB) {												                          \n" \
"	    if (NLMK_BIT_SHIFT == 0) {																			      \n" \
"	        const float r = read_imagef(R, smp, coord2).x;          											  \n" \
"	        const float g = read_imagef(G, smp, coord2).x;														  \n" \
"	        const float b = read_imagef(B, smp, coord2).x;														  \n" \
"	        write_imagef(U1, coord2, (float4) (r, g, b, 1.0f));										              \n" \
"	    } else if (NLMK_BIT_SHIFT != 0) {																		  \n" \
"	        const float r = norm(read_imageui(R, smp, coord2).x);      											  \n" \
"	        const float g = norm(read_imageui(G, smp, coord2).x);												  \n" \
"	        const float b = norm(read_imageui(B, smp, coord2).x);												  \n" \
"	        write_imagef(U1, coord2, (float4) (r, g, b, 1.0f));										              \n" \
"	    }																										  \n" \
"	}       																									  \n" \
"}																												  \n" \
"																												  \n" \
"__kernel																										  \n" \
"void NLM_unpack(__write_only image2d_t R, __write_only image2d_t G, __write_only image2d_t B,   				  \n" \
"__read_only image2d_t U1, const int2 dim) { 		    														  \n" \
"																												  \n" \
"	const int x = get_global_id(0);																				  \n" \
"	const int y = get_global_id(1);																				  \n" \
"	if(x >= dim.x || y >= dim.y) return;																		  \n" \
"																												  \n" \
"	const sampler_t smp = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;		        	  \n" \
"	const int2 coord2 = (int2) (x, y);		                            										  \n" \
"																												  \n" \
"	if (NLMK_TCOLOR & COLOR_GRAY) {		    																	  \n" \
"	    if (NLMK_LSB == 1) {																			          \n" \
"	        const int2 coord2_lsb = (int2) (x, y + dim.y);	               										  \n" \
"		    const ushort in = convert_ushort_sat(read_imagef(U1, smp, coord2).x * (float) USHRT_MAX);             \n" \
"		    const float in_msb = convert_float(in >> CHAR_BIT);                                                   \n" \
"		    const float in_lsb = convert_float(in & 0xFF);                                                        \n" \
"			const float val_msb = native_divide(in_msb, (float) UCHAR_MAX);			                    		  \n" \
"			const float val_lsb = native_divide(in_lsb, (float) UCHAR_MAX);					                	  \n" \
"	        write_imagef(R, coord2, (float4) val_msb);   		                                                  \n" \
"	        write_imagef(R, coord2_lsb, (float4) val_lsb);		                                                  \n" \
"	    } else if (NLMK_BIT_SHIFT != 0) {																          \n" \
"		    const ushort val = denorm(read_imagef(U1, smp, coord2).x);                                            \n" \
"	        write_imageui(R, coord2, (uint4) val);		                        								  \n" \
"	    }																										  \n" \
"	} else if (NLMK_TCOLOR & COLOR_YUV) {												                          \n" \
"	    if (NLMK_BIT_SHIFT == 0) {																			      \n" \
"	        if (NLMK_LSB == 0) {																			      \n" \
"	            const float4 val = read_imagef(U1, smp, coord2);           	                                      \n" \
"	            write_imagef(R, coord2, (float4) val.x);											              \n" \
"	            write_imagef(G, coord2, (float4) val.y);											              \n" \
"	            write_imagef(B, coord2, (float4) val.z);											              \n" \
"	        } else if (NLMK_LSB == 1) {																		      \n" \
"	            const int2 coord2_lsb = (int2) (x, y + dim.y);	               									  \n" \
"		        const ushort4 in = convert_ushort4_sat(read_imagef(U1, smp, coord2) * (float4) USHRT_MAX);        \n" \
"		        const float4 in_msb = convert_float4(in >> (ushort4) CHAR_BIT);                                   \n" \
"		        const float4 in_lsb = convert_float4(in & (ushort4) 0xFF);                                        \n" \
"		    	const float4 val_msb = native_divide(in_msb, (float4) UCHAR_MAX);	                              \n" \
"			    const float4 val_lsb = native_divide(in_lsb, (float4) UCHAR_MAX);                       		  \n" \
"	            write_imagef(R, coord2, (float4) val_msb.x);   	                                                  \n" \
"	            write_imagef(G, coord2, (float4) val_msb.y);   	                                                  \n" \
"	            write_imagef(B, coord2, (float4) val_msb.z);   	                                                  \n" \
"	            write_imagef(R, coord2_lsb, (float4) val_lsb.x);	                                              \n" \
"	            write_imagef(G, coord2_lsb, (float4) val_lsb.y);	                                              \n" \
"	            write_imagef(B, coord2_lsb, (float4) val_lsb.z);	                                              \n" \
"	        }										                    									      \n" \
"	    } else if (NLMK_BIT_SHIFT != 0) {																		  \n" \
"		    const ushort4 val = denorm4(read_imagef(U1, smp, coord2));                                            \n" \
"	        write_imageui(R, coord2, (uint4) val.x);		                    								  \n" \
"	        write_imageui(G, coord2, (uint4) val.y);							                    	          \n" \
"	        write_imageui(B, coord2, (uint4) val.z);							                                  \n" \
"	    }																										  \n" \
"	} else if (NLMK_TCOLOR & COLOR_RGB) {	    										                          \n" \
"	    if (NLMK_BIT_SHIFT == 0) {																			      \n" \
"	        const float4 val = read_imagef(U1, smp, coord2);              	                                      \n" \
"	        write_imagef(R, coord2, (float4) val.x);												              \n" \
"	        write_imagef(G, coord2, (float4) val.y);												              \n" \
"	        write_imagef(B, coord2, (float4) val.z);												              \n" \
"	    } else if (NLMK_BIT_SHIFT != 0) {																		  \n" \
"		    const ushort4 val = denorm4(read_imagef(U1, smp, coord2));                                            \n" \
"	        write_imageui(R, coord2, (uint4) val.x);		                    								  \n" \
"	        write_imageui(G, coord2, (uint4) val.y);							                    	          \n" \
"	        write_imageui(B, coord2, (uint4) val.z);							                                  \n" \
"	    }																										  \n" \
"	}       																									  \n" \
"}																												  ";

#endif //__KERNEL_H__

#include <va/va_x11.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef struct {
    char                input_file[100];
    char                output_file[100];;
    unsigned char       *src_buffer;
    unsigned int        src_format;
    unsigned int        src_color_standard;
    unsigned int        src_buffer_size;
    unsigned int        src_width;
    unsigned int        src_height;
    unsigned int        dst_width;
    unsigned int        dst_height;
    unsigned int        dst_format;
    unsigned int        frame_num;
} VPP_ImageInfo;

    Display        *x11_display;
    VADisplay      _va_dpy;
    VAContextID    _context_id;
    VASurfaceID    out_surface,in_surface;
    VABufferID     vp_pipeline_outbuf = VA_INVALID_ID;
    VABufferID     vp_pipeline_inbuf = VA_INVALID_ID;
    VPP_ImageInfo  vpp_Imageinfo;
	FILE *fOut,*fIn;

bool copyToVaSurface( VASurfaceID surface_id )
{
    VAImage       va_image;
    VAStatus      va_status;
    void          *surface_p = NULL;
    unsigned char *src_buffer;
    unsigned char *y_src, *u_src, *v_src;

    unsigned char *y_dst, *u_dst, *v_dst;
    int           y_size = vpp_Imageinfo.src_width * vpp_Imageinfo.src_height;
    int           i;


    src_buffer = vpp_Imageinfo.src_buffer;

    va_status = vaDeriveImage(_va_dpy, surface_id, &va_image);
    va_status = vaMapBuffer(_va_dpy, va_image.buf, &surface_p);
    printf ("0x%x\t",va_image.format.fourcc);

    switch (va_image.format.fourcc)
    {
        case VA_FOURCC_NV12:

	        y_src = src_buffer;
	        u_src = src_buffer + y_size; // UV offset for NV12

	        y_dst = (unsigned char*)surface_p + va_image.offsets[0];
	        u_dst = (unsigned char*)surface_p + va_image.offsets[1]; // U offset for NV12

	        // Y plane
	        for (i = 0; i < vpp_Imageinfo.src_height; i++)
	        {
	            memcpy(y_dst+va_image.pitches[0]*i, y_src, vpp_Imageinfo.src_width);
	            y_src += vpp_Imageinfo.src_width;
	        }
			// UV offset for NV12
	        for (i = 0; i < vpp_Imageinfo.src_height >> 1; i++)
	        {
	            memcpy(u_dst+va_image.pitches[0]*i, u_src, vpp_Imageinfo.src_width);
	            u_src += vpp_Imageinfo.src_width;
	        }
	        break;

        case VA_FOURCC_YV12:

        	y_src = src_buffer;
        	v_src = src_buffer + y_size;
        	u_src = src_buffer +(y_size*5/4);

	        y_dst = (unsigned char*)surface_p + va_image.offsets[0];
	        v_dst = (unsigned char*)surface_p + va_image.offsets[1]; // v offset for YV12
	        u_dst = (unsigned char*)surface_p + va_image.offsets[2];
	        // Y plane
	        for (i = 0; i < vpp_Imageinfo.src_height; i++)
	        {
	            memcpy(y_dst+va_image.pitches[0]*i, y_src, vpp_Imageinfo.src_width);
	            y_src += vpp_Imageinfo.src_width;
	        }
			// V and U plane
	        for (i = 0; i < vpp_Imageinfo.src_height/2; i++)
	        {
	            memcpy(v_dst,v_src, vpp_Imageinfo.src_width/2);
	            memcpy(u_dst,u_src, vpp_Imageinfo.src_width/2);
	            v_src +=vpp_Imageinfo.src_width/2;
				u_src +=vpp_Imageinfo.src_width/2;
				v_dst +=va_image.pitches[1];
				u_dst +=va_image.pitches[2];

	        }
        	break;

        case VA_FOURCC_YUY2:

	        y_src =src_buffer;

	        y_dst = (unsigned char*)surface_p + va_image.offsets[0];

	        for (i = 0; i < vpp_Imageinfo.src_height; i++)
	        {
	            memcpy(y_dst, y_src, vpp_Imageinfo.src_width*2);
	            y_src += vpp_Imageinfo.src_width*2;
	            y_dst += va_image.pitches[0];
	        }
	    	break;

	case VA_FOURCC_ARGB:

	        y_src =src_buffer;

	        y_dst = (unsigned char*)surface_p + va_image.offsets[0];

	        for (i = 0; i < vpp_Imageinfo.src_height; i++)
	        {
	            memcpy(y_dst, y_src, vpp_Imageinfo.src_width*4);
	            y_src += vpp_Imageinfo.src_width*4;
	            y_dst += va_image.pitches[0];
	        }
	    	break;

    	default:
 	       va_status = VA_STATUS_ERROR_INVALID_IMAGE_FORMAT;
    }

    vaUnmapBuffer(_va_dpy, va_image.buf);
    vaDestroyImage(_va_dpy, va_image.image_id);
    if (va_status != VA_STATUS_SUCCESS)
        return false;
    else
        return true;
}

bool copyFrameFromSurface(VASurfaceID surface_id)
{
    VAStatus      va_status;
    VAImage va_image;
    char *image_data=NULL;
    //FILE *fOut=fopen(vpp_Imageinfo.output_file,"wb");
    va_status = vaDeriveImage(_va_dpy, surface_id, &va_image );
    assert ( va_status == VA_STATUS_SUCCESS);
    va_status = vaMapBuffer(_va_dpy, va_image.buf, (void **)&image_data);
    assert ( va_status == VA_STATUS_SUCCESS);

    printf ("0x%x\n",va_image.format.fourcc);
    switch (va_image.format.fourcc) {
    case VA_FOURCC_NV12:
    case VA_FOURCC_NV21:
	{
        char *pY = ( char *)image_data +va_image.offsets[0];
		char *pUV = ( char *)image_data + va_image.offsets[1];
   	    for(int i = 0; i < vpp_Imageinfo.dst_height; i++)
   	    {
		    fwrite(pY, 1, vpp_Imageinfo.dst_width, fOut);
		    pY += va_image.pitches[0];
   	    }
   	//UV
   	    for(int i = 0; i < vpp_Imageinfo.dst_height/2; i++)
   	    {
		    fwrite(pUV, 1, vpp_Imageinfo.dst_width, fOut);
		    pUV += va_image.pitches[1];
   	    }	 
      	break;
	}
	
	case VA_FOURCC_YV12:
	{
        char *pY = ( char *)image_data +va_image.offsets[0];
		char *pV = ( char *)image_data + va_image.offsets[1];
		char *pU = ( char *)image_data + va_image.offsets[2];
   	    for(int i = 0; i < vpp_Imageinfo.dst_height; i++)
   	    {
		    fwrite(pY, 1, vpp_Imageinfo.dst_width, fOut);
		    pY += va_image.pitches[0];
   	    }
   	//UV
   	    for(int i = 0; i < vpp_Imageinfo.dst_height/2; i++)
   	    {
		    fwrite(pV, 1, vpp_Imageinfo.dst_width/2, fOut);
		    pV += va_image.pitches[1];
   	    }
   	    for(int i = 0; i < vpp_Imageinfo.dst_height/2; i++)
   	    {
		    fwrite(pV, 1, vpp_Imageinfo.dst_width/2, fOut);
		    pU += va_image.pitches[2];
   	    }	 
      	break;
	}

	case VA_FOURCC_YUY2:
	{
        char *pdst = ( char *)image_data +va_image.offsets[0];
   	    for(int i = 0; i < vpp_Imageinfo.dst_height; i++)
   	    {
		    fwrite(pdst, 1, vpp_Imageinfo.dst_width*2, fOut);
		    pdst += va_image.pitches[0];
   	    }
      	break;
	}
    
	case VA_FOURCC_ARGB:
	{
        char *pdst = ( char *)image_data +va_image.offsets[0];
   	    for(int i = 0; i < vpp_Imageinfo.dst_height; i++)
   	    {
		    fwrite(pdst, 1, vpp_Imageinfo.dst_width*4, fOut);
		    pdst += va_image.pitches[0];
   	    }
      	break;
	}
    default:
    return false;
    }
    va_status = vaUnmapBuffer(_va_dpy, va_image.buf);
    va_status = vaDestroyImage(_va_dpy,va_image.image_id);
    return true;
}

bool processFrame()
{
    //VAStatus va_status;
	vaBeginPicture(_va_dpy, _context_id, out_surface);
	vaRenderPicture(_va_dpy, _context_id,&vp_pipeline_inbuf, 1);
	vaEndPicture(_va_dpy, _context_id);
	copyFrameFromSurface(out_surface);
	return true;
}

bool createOutputSurface()
{
    VASurfaceAttrib surfaceAttrib;
    VARectangle targetRect;
    VAProcPipelineParameterBuffer vpOutparam;

    surfaceAttrib.type = VASurfaceAttribPixelFormat;
    surfaceAttrib.value.type = VAGenericValueTypeInteger;
    surfaceAttrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
    int surface_width, surface_height;
    surface_width = vpp_Imageinfo.dst_width;
    surface_height = vpp_Imageinfo.dst_height;
    if ( vpp_Imageinfo.dst_format == 4 )
	{
	    surfaceAttrib.value.value.i = VA_FOURCC_ARGB;
	}
	else if ( vpp_Imageinfo.dst_format == 3 )
	{
	    surfaceAttrib.value.value.i = VA_FOURCC_YUY2;
	}
	else if ( vpp_Imageinfo.dst_format == 2 )
	{
	    surfaceAttrib.value.value.i = VA_FOURCC_YV12;
	}
	else if ( vpp_Imageinfo.dst_format == 1 )
	{
		surfaceAttrib.value.value.i = VA_FOURCC_NV12;
	}

    vaCreateSurfaces(_va_dpy,
					VA_RT_FORMAT_YUV420,
					surface_width,
					surface_height,
					&out_surface,
					1,    //Allocating one surface per layer at a time
					&surfaceAttrib,
					//NULL,
					1);

    targetRect.x = 0;
    targetRect.y = 0;
    targetRect.width = vpp_Imageinfo.dst_width;
    targetRect.height = vpp_Imageinfo.dst_height;

    vpOutparam.output_region = &targetRect;
    vpOutparam.surface_region = &targetRect;
    vpOutparam.surface = out_surface;


    vaCreateBuffer(_va_dpy,
		        _context_id,
		        VAProcPipelineParameterBufferType,
		        sizeof(VAProcPipelineParameterBuffer),
		        1,
		        &vpOutparam,
		        &vp_pipeline_outbuf);


    return true;
}

bool createSurface()
{
    int countFrames=0;
    int numbytes=3/2;
    VASurfaceAttrib surfaceAttrib;
    VAProcPipelineParameterBuffer vpInputParam;
    VAStatus    vaStatus;
    surfaceAttrib.type = VASurfaceAttribPixelFormat;
    surfaceAttrib.value.type = VAGenericValueTypeInteger;
    surfaceAttrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
    unsigned int surface_width, surface_height;
    surface_width = vpp_Imageinfo.src_width;
    surface_height = vpp_Imageinfo.src_height;
 	 if (vpp_Imageinfo.src_format == 1) //NV12 
	{
		surfaceAttrib.value.value.i = VA_FOURCC_NV12;
	}
	else if (vpp_Imageinfo.src_format == 3) //YUY2
	{
		surfaceAttrib.value.value.i = VA_FOURCC_YUY2;
		numbytes=2;
	}
	else if (vpp_Imageinfo.src_format == 2) //YV12
	{
		surfaceAttrib.value.value.i = VA_FOURCC_YV12;
	}
	else if (vpp_Imageinfo.src_format == 4) //RGB
	{
		surfaceAttrib.value.value.i = VA_FOURCC_ARGB;
		numbytes=4;
	}
	else
	{	printf("Invalid format file\n");
		return false;
	}
    vaStatus=vaCreateSurfaces(_va_dpy,
					VA_RT_FORMAT_YUV420,
					surface_width,
					surface_height,
					&in_surface,
					1,    //Allocating one surface per layer at a time
					&surfaceAttrib,
					//NULL,
					1);
    assert (vaStatus == VA_STATUS_SUCCESS);
    
	

	fIn=fopen(vpp_Imageinfo.input_file,"rb");
	printf("test\n");
	//fseek(fIn, 1, SEEK_SET);use color change
    memset(&vpp_Imageinfo, sizeof(vpp_Imageinfo), sizeof(size_t*));
    vpp_Imageinfo.src_buffer=(unsigned char *)malloc(vpp_Imageinfo.src_width*vpp_Imageinfo.src_height*numbytes);
      
	fOut = fopen(vpp_Imageinfo.output_file, "wb");
	if (fIn == NULL)
	{
		printf ("Failed to open input file \n");
		return false;
	}
	else
		printf("success to open input file\n");
	while(!feof(fIn))
	{
	
		countFrames++;
        fread(vpp_Imageinfo.src_buffer,1,
        	vpp_Imageinfo.src_width*vpp_Imageinfo.src_height*numbytes,fIn);
		copyToVaSurface(in_surface);
    	memset(&vpInputParam, 0, sizeof(VAProcPipelineParameterBuffer));
    	vpInputParam.surface_color_standard  = VAProcColorStandardBT601;
    	vpInputParam.output_background_color = 0;
    	vpInputParam.output_color_standard   = VAProcColorStandardNone;


    	vpInputParam.filters                 = NULL;
    	vpInputParam.num_filters             = 0;
    	vpInputParam.forward_references      = NULL;
    	vpInputParam.num_forward_references  = 0;
    	vpInputParam.backward_references     = 0;
    	vpInputParam.num_backward_references = 0;
    	vaStatus=vaCreateBuffer(_va_dpy,
									_context_id,
									VAProcPipelineParameterBufferType,
									sizeof(VAProcPipelineParameterBuffer),
									1,
									&vpInputParam,
									&vp_pipeline_inbuf);
    	assert (vaStatus == VA_STATUS_SUCCESS);

		createOutputSurface();
		processFrame();

	}
	fclose (fIn);
	fclose (fOut);
	printf ("No of Frames: %d \n",countFrames);
	return true;
}

bool libvaInit()
{
    int          major_ver, minor_ver;
    const char  *driver=NULL;
    VAConfigAttrib  vpAttrib;
    VAConfigID cfg;
    VAStatus    vaStatus;

    x11_display = XOpenDisplay(":0.0");
    if (NULL == x11_display) {
       printf("Error: Can't connect X server! %s %s(line %d)\n", __FILE__, __func__, __LINE__);
       return false;
    }
    _va_dpy = vaGetDisplay(x11_display);

    vaStatus = vaInitialize(_va_dpy, &major_ver, &minor_ver);
    if (vaStatus != VA_STATUS_SUCCESS) {
        printf("Error: Failed vaInitialize(): in %s %s(line %d)\n", __FILE__, __func__, __LINE__);
        return false;
    }
    printf("libva version: %d.%d\n",major_ver,minor_ver);

    driver = vaQueryVendorString(_va_dpy);
    printf("VAAPI Init complete; Driver version :%s\n",driver);

    memset(&vpAttrib, 0, sizeof(vpAttrib));
    vpAttrib.type  = VAConfigAttribRTFormat;
    vpAttrib.value = VA_RT_FORMAT_YUV420;
    vaStatus=vaCreateConfig(_va_dpy, VAProfileNone,
                                VAEntrypointVideoProc,
                                &vpAttrib,
                                1, &cfg);
    assert (vaStatus == VA_STATUS_SUCCESS);
    vaStatus=vaCreateContext(_va_dpy, cfg, vpp_Imageinfo.src_width, vpp_Imageinfo.src_height, 0, NULL, 0, &_context_id);
    assert (vaStatus == VA_STATUS_SUCCESS);

    return true;
}

bool libvaTerminate()
{
    vaTerminate(_va_dpy);
    XCloseDisplay(x11_display);
    _va_dpy = x11_display = NULL;
    return true;
}


bool parseCommand( int argc, char** argv)
{
  	vpp_Imageinfo.src_format = 0;
    for (int arg_idx = 1; arg_idx < argc - 1; arg_idx += 2)
    {
	if (!strcmp(argv[arg_idx], "-i"))
	{
	    strncpy(vpp_Imageinfo.input_file, argv[arg_idx + 1], 100);
	}
	else if (!strcmp(argv[arg_idx], "-iw"))
	{
	    vpp_Imageinfo.src_width = atoi(argv[arg_idx + 1]);
	}
	else if (!strcmp(argv[arg_idx], "-ih"))
	{
	    vpp_Imageinfo.src_height = atoi(argv[arg_idx + 1]);
	}
	else if (!strcmp(argv[arg_idx], "-if"))
	{
		 vpp_Imageinfo.src_format = atoi(argv[arg_idx + 1]);
	}
	else if (!strcmp(argv[arg_idx], "-o"))
	{
	    strncpy(vpp_Imageinfo.output_file, argv[arg_idx + 1], 100);
	}
	else if (!strcmp(argv[arg_idx], "-ow"))
	{
	    vpp_Imageinfo.dst_width = atoi(argv[arg_idx + 1]);
	}
	else if (!strcmp(argv[arg_idx], "-oh"))
	{
	    vpp_Imageinfo.dst_height = atoi(argv[arg_idx + 1]);
	}
  	else if (!strcmp(argv[arg_idx], "-of"))
	{
	    vpp_Imageinfo.dst_format = atoi(argv[arg_idx + 1]);
	}
 	else if (!strcmp(argv[arg_idx], "-if"))
	{	
 	    vpp_Imageinfo.src_format = atoi(argv[arg_idx + 1]);
 	}
	else
	{
         //   print_help();
	}
    }
    return true;
}


int main(int argc, char** argv)
{  
  if (( argc % 2 == 0 ) || ( argc == 1 ))
    {
        printf("wrong parameters count\n");
        printf("./vpp -i <input file> -iw <input height> -ih <input height> -if <1/2/3/4> -o <output file> -ow <output width> -oh <output height> -of <1/2/3/4>\n");
        printf("1:NV12\n2:YV12\n3:YUY2\n4:ARGB\n");
        return -1;
    }
	parseCommand(argc, argv);
	libvaInit();
	createSurface();
	libvaTerminate();

    return 0;
}

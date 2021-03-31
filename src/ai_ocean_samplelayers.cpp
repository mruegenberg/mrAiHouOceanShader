/*
 * Houdini ocean shader for Arnold. Evaluates Houdini oceans using CVEX
 */

#include <ai.h>
#include <string.h>
#include <cstdio>
#include <CVEX/CVEX_Context.h>
#include <UT/UT_Vector3.h>
#include <MOT/MOT_Director.h>

enum AiHOceanParms {    
    p_filename,
    p_maskname,
    p_restname,
    p_time,
    p_depthfalloff,
    p_falloff,
    p_downsample,
};

#define TRUE 1
#define FALSE 0

AI_SHADER_NODE_EXPORT_METHODS(AiHOceanShdMethods);

static const char *depthfalloffmodenames[] =
{
    "none", "exponential", "exponentialbyfreq", NULL
};

node_parameters
{        
    AiParameterStr("filename", "");
    AiParameterStr("maskname", "");
    AiParameterStr("restname", "rest");
    AiParameterFlt("time", 0);
    AiParameterEnum("depthfalloff", 0, depthfalloffmodenames);
    AiParameterFlt("falloff", 1);
    AiParameterInt("downsample", 0);
}

struct ShaderData {
    AtCritSec critSec;
    
    char codeFilename[200]; // temporary file for compiled VEX code
    CVEX_Context* ctxs[AI_MAX_THREADS]; // at most one context per thread

    // uniform data - needs to be persistent
    fpreal32 fltBuffersU[AI_MAX_THREADS][2];
    int32 intBuffersU[AI_MAX_THREADS][2];
    CVEX_StringArray filenameBuffer[AI_MAX_THREADS];
    CVEX_StringArray masknameBuffer[AI_MAX_THREADS];
    
};

node_initialize
{
    // we use placement new syntax to have both the constructor (partcularly for CVEX_Context)
    // called and memory allocated by Arnold's internal malloc
    // by Arnold
    ShaderData* data = new (AiMalloc(sizeof(ShaderData))) ShaderData();
    for (int i=0; i<AI_MAX_THREADS; ++i)
    {
        data->ctxs[i] = 0;

        memset(data->fltBuffersU[i], 0, 2*sizeof(fpreal32));
        memset(data->intBuffersU[i], 0, 2*sizeof(int32));
        data->filenameBuffer[i] = CVEX_StringArray();
        data->masknameBuffer[i] = CVEX_StringArray();
    }
    AiCritSecInit(&(data->critSec));
    AiNodeSetLocalData(node, data);
}

node_update
{
    ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);
    for (int i=0; i<AI_MAX_THREADS; ++i) // invalidate all ctxs
    {
        if (data->ctxs[i])
        {
            delete data->ctxs[i];
            data->ctxs[i] = 0;
        }
    }
}

node_finish
{
    ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);

    if (data)
    {
        AiCritSecClose(&(data->critSec));
        data->~ShaderData(); // call constructor without dealloc
        AiFree((void*) data);
        AiNodeSetLocalData(node, NULL);
    }
}

node_loader
{
    if (i > 0) return FALSE;
   
    node->methods      = AiHOceanShdMethods;
    node->output_type  = AI_TYPE_RGBA; // displacement + cusp info. currently we have no way of getting velocity and cuspdir out. 
    node->name         = "ai_ocean_samplelayers";
    node->node_type    = AI_NODE_SHADER;
    strcpy(node->version, AI_VERSION);
    return TRUE;
};


shader_evaluate
{
    sg->out.RGBA() = AI_RGB_RED; // dummy

    ShaderData* data = (ShaderData*) AiNodeGetLocalData(node);

    // context. only initialized once per thread
    CVEX_Context *ctx = data->ctxs[sg->tid];
    if (!ctx)
    {
        ctx = new CVEX_Context();
        data->ctxs[sg->tid] = ctx;

        // temporary file for VEX code
        const char *fileName = "/tmp";
        
        AiCritSecEnter(&(data->critSec)); // tmpnam is not threadsafe

        strcpy(data->codeFilename, "/tmp/oceanVexcodeXXXXXX");
#ifdef _WIN32
        _mktemp_s(data->codeFilename, strlen(data->codeFilename));
#else
        mkstemp(data->codeFilename);
#endif
        // std::tmpnam(data->codeFilename); // TODO: add pragma to silence the unsafe warning?
        strncat(data->codeFilename, ".vfl", 150);
        fileName = data->codeFilename;
        AiMsgDebug("[AiOcean] CVEX tmp filename: %s", fileName);

        const char *vexCode =
            "#include <ocean.h>\n"
            "cvex cvex1(const string filename=''; const string maskname=''; const vector P={0,0,0}; const float time=0; const int depthfalloff=0; const float falloff=1; const int downsample=0;\n"
            "export vector displacement={0,0,0}; export vector velocity={0,0,0}; export float cusp=0; export vector cuspdir={0,0,0}) {\n"
            "        oceanSampleLayers(filename, maskname, time, P, 0, depthfalloff, falloff, downsample, displacement, velocity, cusp, cuspdir);\n"
            "}\n\n";
        
        std::ofstream out(fileName);
        out << vexCode;
        out.close();

        UT_String script(fileName); // copy fileName to a UT_String (internal Houdini string)
        char *argv[1024];
        int argc = script.parse(argv, 1024); // converts so that VEX context can load it


        AiCritSecLeave(&(data->critSec));
        
        // default inputs translated from default Arnold names to default Hou names
        ctx->addInput("P",    CVEX_TYPE_VECTOR3, true); // P
        ctx->addInput("Eye",  CVEX_TYPE_VECTOR3, true); // Ro
        ctx->addInput("I",    CVEX_TYPE_VECTOR3, true); // Rd
        ctx->addInput("dPds", CVEX_TYPE_VECTOR3, true); // dPdu
        ctx->addInput("dPdt", CVEX_TYPE_VECTOR3, true); // dPdv
        ctx->addInput("N",    CVEX_TYPE_VECTOR3, true); // N
        ctx->addInput("Ng",   CVEX_TYPE_VECTOR3, true); // Ng
        ctx->addInput("s",    CVEX_TYPE_FLOAT,   true); // u
        ctx->addInput("t",    CVEX_TYPE_FLOAT,   true); // v
        
        // ocean-specific parms
        ctx->addInput("time", CVEX_TYPE_FLOAT, false); // false = not varying = same for all pts
        ctx->addInput("filename", CVEX_TYPE_STRING, false);
        ctx->addInput("maskname", CVEX_TYPE_STRING, false);
        ctx->addInput("depthfalloff", CVEX_TYPE_INTEGER, false);
        ctx->addInput("falloff", CVEX_TYPE_FLOAT, false);
        ctx->addInput("downsample", CVEX_TYPE_INTEGER, false);

        
        if(! ctx->load(argc, argv)) {
            const char *err = ctx->getLastError();
            AiMsgWarning("[AiOcean] loading the CVEX context failed: %s", err);
            return;
        }



        /////////////// Add uniform parms and set their values
        const char *spectrumfilename = AiShaderEvalParamStr(p_filename);
        const char *spectrummask = AiShaderEvalParamStr(p_maskname);
        float time = AiShaderEvalParamFlt(p_time);
        int depthfalloff = AiShaderEvalParamEnum(p_depthfalloff);
        float falloff = AiShaderEvalParamFlt(p_falloff);
        int downsample = AiShaderEvalParamInt(p_downsample);

        CVEX_Value
            *filename_val, *maskname_val,
            *time_val, *depthfalloff_val, *falloff_val, *downsample_val;
        {
            filename_val = ctx->findInput("filename", CVEX_TYPE_STRING);
            maskname_val = ctx->findInput("maskname", CVEX_TYPE_STRING);       
            time_val = ctx->findInput("time", CVEX_TYPE_FLOAT);
            depthfalloff_val = ctx->findInput("depthfalloff", CVEX_TYPE_INTEGER);
            falloff_val = ctx->findInput("falloff", CVEX_TYPE_FLOAT);
            downsample_val = ctx->findInput("downsample", CVEX_TYPE_INTEGER);

            if (filename_val)
            {
                data->filenameBuffer[sg->tid].append(spectrumfilename);
                filename_val->setTypedData(data->filenameBuffer[sg->tid]);
            }
            if(maskname_val) {
                data->masknameBuffer[sg->tid].append(spectrummask);
                maskname_val->setTypedData(data->masknameBuffer[sg->tid]);
            }           
            if (time_val)
            {
                data->fltBuffersU[sg->tid][0] = time;
                time_val->setTypedData(data->fltBuffersU[sg->tid] + 0, 1);
            }
            if(depthfalloff_val)
            {
                data->intBuffersU[sg->tid][0] = depthfalloff;
                depthfalloff_val->setTypedData(data->intBuffersU[sg->tid] + 0, 1);
            }
            if(falloff_val)
            {
                data->fltBuffersU[sg->tid][1] = falloff;
                falloff_val->setTypedData(data->fltBuffersU[sg->tid] + 1, 1);
            }
            if(downsample_val)
            {
                data->intBuffersU[1][sg->tid] = downsample;
                downsample_val->setTypedData(data->intBuffersU[sg->tid] + 1, 1);
            }
        }

    }

    // AiMsgWarning("after context %s", AiGetCompileOptions());


    UT_Vector3 vecBuffers[1] = {UT_Vector3(0,0,0)}; // for simplicity, one vec buffer for all
    // TODO: maybe don't reallocate this in each call? CVEX_Value probably just keeps a pointer,
    //       so we'd have to do the setTypedData call only once, saving both on [stack] alloc and unneeded
    //       assigns.

    // Shading Group params
    CVEX_Value
        *P_val;
    bool useRest = true;
    AtVector restP = sg->P;

    {
        P_val    = ctx->findInput("P",    CVEX_TYPE_VECTOR3);
        
        if (P_val)
        {
            const char *restname = AiShaderEvalParamStr(p_restname);
            static const AtString rest(restname);
            if(useRest && AiUDataGetVec(rest, restP)) {
                vecBuffers[0].assign(&(restP.x));
            }
            else {
                useRest = false; // rest not found, so don't use it downstream
                vecBuffers[0].assign(&(sg->P.x));
            }
            P_val->setTypedData(vecBuffers + 0, 1);
        }
    }
    
    CVEX_Value *out_displacement_val; // CVEX_Value *out_velocity_val;
    CVEX_Value *out_cusp_val; // CVEX_Value *out_cuspdir_val;
    UT_Vector3 out_displacement[1] = {UT_Vector3(0,0,0)};
    // UT_Vector3 out_velocity[1];
    fpreal32 out_cusp[1] = {0};
    // UT_Vector3 out_cuspdir[1];
    out_displacement_val = ctx->findOutput("displacement", CVEX_TYPE_VECTOR3);
    // out_velocity_val = ctx->findOutput("velocity", CVEX_TYPE_VECTOR3);
    out_cusp_val = ctx->findOutput("cusp", CVEX_TYPE_FLOAT);
    // out_cuspdir_val = ctx->findOutput("cuspdir", CVEX_TYPE_VECTOR3);
    if (out_displacement_val && out_cusp_val) {
        out_displacement_val->setTypedData(out_displacement, 1);
        out_cusp_val->setTypedData(out_cusp, 1);
    }

    ctx->run(1, false);

    if(useRest) {
        AtVector preDisp = sg->P - restP;
        sg->out.RGBA().r = out_displacement[0].x() - preDisp.x;
        sg->out.RGBA().g = out_displacement[0].y() - preDisp.y;
        sg->out.RGBA().b = out_displacement[0].z() - preDisp.z;
    }
    else {
        sg->out.RGBA().r = out_displacement[0].x();
        sg->out.RGBA().g = out_displacement[0].y();
        sg->out.RGBA().b = out_displacement[0].z();
    }
    sg->out.RGBA().a = out_cusp[0];
}


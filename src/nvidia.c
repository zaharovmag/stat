#include <stdio.h>
#define HAVE_NVIDIA
#include <stat_common.h>

#define  __NV_MAX_DEV 2

typedef struct __device_link device_link;
struct __device_link
{
  device_link * next;
  nvmlDevice_t * __nv_dev;
};

typedef struct __device_link_head device_link_head;
struct __device_link_head
{
  device_link *first;
  device_link *last;
  device_link *arr_ptr;
};

typedef struct __nv_ctx nv_ctx;
struct __nv_ctx{
  unsigned int device_count;
  device_link_head * dev_head;
};

#define __NVML_SUCCESS(x) (x == NVML_SUCCESS)

#define NV_S(x) __NVML_SUCCESS(x)

#define LOG_ERROR(x) \
  fprintf(stderr,"%s:%d Error: %s\n",__FILE__,__LINE__,x)

#define NV_CK(x,y)  \
  if(!NV_S(x))   {  \
    LOG_ERROR(y);   \
  }



int fix_unsupported_bug(nvmlDevice_t device)
{
    unsigned int *fix = (unsigned int *)device;
    char version[NVML_SYSTEM_NVML_VERSION_BUFFER_SIZE];

    if (nvmlSystemGetDriverVersion(version,
                                  NVML_SYSTEM_NVML_VERSION_BUFFER_SIZE)
      != NVML_SUCCESS)
      return 1;

    if (!strcmp(version, "1325.08") ||
        !strcmp(version, "1319.32") ||
        !strcmp(version, "1319.23") ||
        !strcmp(version, "325.15"))
    {
      #ifdef __i386__
        fix[201] = 1;
        fix[202] = 1;
      #else
        fix[202] = 1;
        fix[203] = 1;
      #endif
        return 0;
    }

  return 1;
}

static void _nv_init(nv_ctx * ctx)
{
  int ret;

  ret = nvmlInit();
  NV_CK(ret,"nvmlInit()");


  ret = nvmlDeviceGetCount(&ctx->device_count);
  NV_CK(ret,"GetDeviceCOunt()\n");
}

static void _nv_shutdown(nv_ctx *ctx)
{
}




int main()
{
  nv_ctx *ctx = malloc(sizeof(nv_ctx));

  _nv_init(ctx);

  printf("nvidia: dev_count: %d\n",ctx->device_count);


  _nv_shutdown(ctx);
}

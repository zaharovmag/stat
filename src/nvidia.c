#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stat_common.h>

#define  __NV_DEF_DEV_COUNT 2
#define FOR_EACH(x) for(int i = 0;i < x;i++)

typedef struct __device_link device_link;

struct __device_link
{
  device_link * next;
  nvmlDevice_t * __nv_dev;
  int devno;
  unsigned int pci_gen;
  unsigned int cur_pci_width;
  unsigned long long gpu_memory_total;
  unsigned long long last_gpu_mem_free;
  unsigned long long last_gpu_mem_used;
  unsigned int last_gpu_ut;
  unsigned int last_gpu_mem_ut;
};

typedef struct __device_head device_head;

struct __device_head
{
  device_link *first;
  device_link *last;
  device_link **arr_ptr;
  unsigned int arr_len;
  unsigned int arr_cur;
};

typedef struct __nv_ctx nv_ctx;

struct __nv_ctx{
  unsigned int devs;
  device_head * dh;
};

#define DEV_LINK(ctx,no) \
    ctx->dh->arr_ptr[no]

#define __NVML_SUCCESS(x) (x == NVML_SUCCESS)

#define NV_S(x) __NVML_SUCCESS(x)

#define LOG_ERROR(x) \
  fprintf(stderr,"%s:%d Error: %s\n",__FILE__,__LINE__,x)

#define NV_CK(x,y)  \
  if(!NV_S(x))   {  \
    LOG_ERROR(y);   \
  }



static device_head *
__alloc_dev_head()
{
  device_head *dh = NULL;

  dh = malloc(sizeof(device_head));
  dh->first = NULL;
  dh->last = NULL;

  dh->arr_ptr = malloc(sizeof(void *) * __NV_DEF_DEV_COUNT );
  if (!dh->arr_ptr){
    LOG_ERROR("malloc error\n");
    free(dh);
    return NULL;
  }
  dh->arr_len = __NV_DEF_DEV_COUNT;
  dh->arr_cur = 0;

  return dh;
}


static void
__dev_add(nv_ctx * ctx,device_link *link)
{
  device_head * dh;

  if (!ctx || !ctx->devs)
    abort();

  dh = ctx->dh;

  if (!dh->first){
    dh->first = link;
    dh->first->next = link;
    dh->last = link;
    dh->arr_ptr[0] = (void *)link;
    dh->arr_cur ++;
  } else {
    /*TODO extend array*/
    dh->first->next = link;
    dh->last = link;
    dh->arr_ptr[dh->arr_cur] =  link;
    dh->arr_cur++;
  }
}

static device_link *
__alloc_dev(int i)
{
    device_link *dl;
    dl = malloc(sizeof(device_link));
    if (!dl){
	    LOG_ERROR("alloc error\n");
	    return NULL;
    }

    dl->devno = i;
    dl->next = NULL;
    dl->__nv_dev = malloc(sizeof(nvmlDevice_t));
    if (!dl->__nv_dev){
      LOG_ERROR("alloc error\n");
      free(dl);
      return NULL;
    }
    return dl;
}

static void
__nv_ctx_free(nv_ctx *ctx)
{
    if (!ctx)
	    abort();

    FOR_EACH(ctx->devs){
	    free(ctx->dh->arr_ptr[i]);
    }
    free(ctx->dh->arr_ptr);
    free(ctx->dh);
    free(ctx);
}


static int
__fix_unsupported_bug(nvmlDevice_t device)
{
    unsigned int *fix = (unsigned int *)device;
    char version[NVML_SYSTEM_NVML_VERSION_BUFFER_SIZE];

    if (nvmlSystemGetDriverVersion(version,
                                  NVML_SYSTEM_NVML_VERSION_BUFFER_SIZE)
      != NVML_SUCCESS)
      return 1;

    if (!strcmp(version, "325.08") ||
        !strcmp(version, "319.32") ||
        !strcmp(version, "319.23") ||
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



void
nv_init(nv_ctx * ctx)
{
  int ret;
  device_link *dl;

  ret = nvmlInit();
  NV_CK(ret,"nvmlInit()");


  ret = nvmlDeviceGetCount(&ctx->devs);
  NV_CK(ret,"GetDeviceCOunt()\n");
  ctx->dh = __alloc_dev_head();
  if (!ctx->dh)
      abort();

  FOR_EACH(ctx->devs){
      __dev_add(ctx,__alloc_dev(i));
      dl = ctx->dh->arr_ptr[i];

      ret = nvmlDeviceGetHandleByIndex(i,dl->__nv_dev);
      NV_CK(ret,"nvmlDeviceGet");
      ret = __fix_unsupported_bug(*dl->__nv_dev);

      ret = nvmlDeviceGetCurrPcieLinkGeneration(*dl->__nv_dev,&dl->pci_gen);
      NV_CK(ret,"getPciGen");

      ret = nvmlDeviceGetCurrPcieLinkWidth(*dl->__nv_dev,
					 &dl->cur_pci_width);

      NV_CK(ret,"getPciWidth");

  }
}



void
nv_shutdown(nv_ctx *ctx)
{
    nvmlShutdown();
    __nv_ctx_free(ctx);
}



int
nv_gather_stat(nv_ctx *ctx,int devno)
{
    int ret;
    device_link *dl;
    nvmlUtilization_t ut;
    nvmlMemory_t mi;

    if (!ctx)
      abort();
    dl = ctx->dh->arr_ptr[devno];

    ret = nvmlDeviceGetUtilizationRates(*dl->__nv_dev,&ut);
    NV_CK(ret,"getUtilization");
    dl->last_gpu_ut = ut.gpu;
    dl->last_gpu_mem_ut = ut.memory;

    ret = nvmlDeviceGetMemoryInfo(*dl->__nv_dev,&mi);
    NV_CK(ret,"getMemoryInfo");
    dl->gpu_memory_total = mi.total;
    dl->last_gpu_mem_free = mi.free;
    dl->last_gpu_mem_used = mi.used;

    return 1;
}

int
nv_get_gpu_ut(nv_ctx *ctx,int devno)
{
    return DEV_LINK(ctx,devno)->last_gpu_ut;
}

int
nv_get_gpu_mem_ut(nv_ctx *ctx,int devno)
{
    return DEV_LINK(ctx,devno)->last_gpu_mem_ut;
}

int
nv_get_gpu_mem_usage_pct(nv_ctx *ctx,int devno)
{
    return DEV_LINK(ctx,devno)->last_gpu_mem_used /
                    (DEV_LINK(ctx,devno)->gpu_memory_total/100);
}


#ifdef __TEST_ONLY__
int main()
{
  nv_ctx *ctx = malloc(sizeof(nv_ctx));


  nv_init(ctx);
  nv_gather_stat(ctx,0);
  nv_gather_stat(ctx,1);

  printf("gpu utilization %d%%\n",
	nv_get_gpu_ut(ctx,0));
  printf("gpu memory utilization %d%%\n",
	nv_get_gpu_mem_ut(ctx,0));

  printf("gpu memory usage %d%%\n",
	nv_get_gpu_mem_usage_pct(ctx,0));

  printf("nvidia: dev_count: %d\n",ctx->devs);


  nv_shutdown(ctx);
}
#endif

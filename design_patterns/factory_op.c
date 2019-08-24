
工厂实现 /////////////1)////////////////////////
alsa音频框架 Linux ALSA驱动框架 接口调用实现音频数据操作具体实现
static pjmedia_aud_dev_factory_op alsa_factory_op =
{
    &alsa_factory_init,
    &alsa_factory_destroy,
    &alsa_factory_get_dev_count,
    &alsa_factory_get_dev_info,
    &alsa_factory_default_param,
    &alsa_factory_create_stream,
    &alsa_factory_refresh
};



/* API: init factory */
static pj_status_t alsa_factory_init(pjmedia_aud_dev_factory *f)
{
    pj_status_t status = alsa_factory_refresh(f);
    if (PJ_SUCCESS != status)
        return status;

    PJ_LOG(4,(THIS_FILE, "ALSA initialized"));
    return PJ_SUCCESS;
}


/* Create ALSA audio driver. */
pjmedia_aud_dev_factory* pjmedia_alsa_factory(pj_pool_factory *pf)  /////////////4)///////key/////////////////
{
    struct alsa_factory *af;
    pj_pool_t *pool;

    pool = pj_pool_create(pf, "alsa_aud_base", 256, 256, NULL);
    af = PJ_POOL_ZALLOC_T(pool, struct alsa_factory);
    af->pf = pf;
    af->base_pool = pool;
    af->base.op = &alsa_factory_op;  /////////////2)////////////////////////

    return &af->base;/////////////3)////////////////////////
}

//////////////////////////////////////////////////////////////////////////

注册工厂,适配不同音频框架
pjmedia_aud_subsys_init
pjmedia_aud_register_factory
pjmedia_aud_unregister_factory

PJ_DEF(pj_status_t) pjmedia_aud_subsys_init(pj_pool_factory *pf)
{
#if PJMEDIA_AUDIO_DEV_HAS_ALSA  //适配alsa音频框架，其他同理
    aud_subsys->drv[drv_idx].create = &pjmedia_alsa_factory;  /////////////5)//////////////key//////////
#endif

    /* Initialize each factory and build the device ID list */
    status = pjmedia_aud_driver_init([drv_idx, PJ_FALSE);  /////////////6)////////////////////////
    if (status != PJ_SUCCESS) {
    pjmedia_aud_driver_deinit(drv_idx);
    }


}




/* API: register an audio device factory to the audio subsystem. */
PJ_DEF(pj_status_t)
pjmedia_aud_register_factory(pjmedia_aud_dev_factory_create_func_ptr adf) //pjmedia_aud_dev_factory* pjmedia_alsa_factory(pj_pool_factory *pf)
{
    /*
    声明
    typedef pjmedia_aud_dev_factory*
    (*pjmedia_aud_dev_factory_create_func_ptr)(pj_pool_factory*);

    pjmedia_aud_dev_factory_create_func_ptr函数 实现
    //pjmedia_aud_dev_factory* pjmedia_alsa_factory(pj_pool_factory *pf)
    */

    pj_status_t status;
    pjmedia_aud_subsys *aud_subsys = pjmedia_get_aud_subsys();

    if (aud_subsys->init_count == 0)
        return PJMEDIA_EAUD_INIT;

    aud_subsys->drv[aud_subsys->drv_cnt].create = adf;
    status = pjmedia_aud_driver_init(aud_subsys->drv_cnt, PJ_FALSE);
    if (status == PJ_SUCCESS) {
        aud_subsys->drv_cnt++;
    } else {
        pjmedia_aud_driver_deinit(aud_subsys->drv_cnt);
    }

    return status;
}





/* API: init driver */
PJ_DEF(pj_status_t) pjmedia_aud_driver_init(unsigned drv_idx,
        pj_bool_t refresh)
{
    pjmedia_aud_driver *drv = &aud_subsys.drv[drv_idx];
    pjmedia_aud_dev_factory *f;
    unsigned i, dev_cnt;
    pj_status_t status;

    if (!refresh && drv->create) {
        /* Create the factory */
        f = (*drv->create)(aud_subsys.pf); //////////////////7)///key////////////////
        if (!f)
            return PJ_EUNKNOWN;

        /* Call factory->init() */
        status = f->op->init(f);   //////////////////8)///key/////// typedef struct pjmedia_aud_dev_factory_op{   pj_status_t (*init)(pjmedia_aud_dev_factory *f);//////
        if (status != PJ_SUCCESS) {
            f->op->destroy(f);
            return status;
        }
    } else {
        f = drv->f;
    }

    if (!f)
        return PJ_EUNKNOWN;

    /* Get number of devices */
    dev_cnt = f->op->get_dev_count(f);
    if (dev_cnt + aud_subsys.dev_cnt > PJMEDIA_AUD_MAX_DEVS) {
        PJ_LOG(4,(THIS_FILE, "%d device(s) cannot be registered because"
                  " there are too many devices",
                  aud_subsys.dev_cnt + dev_cnt -
                  PJMEDIA_AUD_MAX_DEVS));
        dev_cnt = PJMEDIA_AUD_MAX_DEVS - aud_subsys.dev_cnt;
    }

    /* enabling this will cause pjsua-lib initialization to fail when there
     * is no sound device installed in the system, even when pjsua has been
     * run with --null-audio
     *
    if (dev_cnt == 0) {
    f->op->destroy(f);
    return PJMEDIA_EAUD_NODEV;
    }
    */

    /* Fill in default devices */
    drv->play_dev_idx = drv->rec_dev_idx =
                            drv->dev_idx = PJMEDIA_AUD_INVALID_DEV;
    for (i=0; i<dev_cnt; ++i) {
        pjmedia_aud_dev_info info;

        status = f->op->get_dev_info(f, i, &info);
        if (status != PJ_SUCCESS) {
            f->op->destroy(f);
            return status;
        }

        if (drv->name[0]=='\0') {
            /* Set driver name */
            pj_ansi_strncpy(drv->name, info.driver, sizeof(drv->name));
            drv->name[sizeof(drv->name)-1] = '\0';
        }

        if (drv->play_dev_idx < 0 && info.output_count) {
            /* Set default playback device */
            drv->play_dev_idx = i;
        }
        if (drv->rec_dev_idx < 0 && info.input_count) {
            /* Set default capture device */
            drv->rec_dev_idx = i;
        }
        if (drv->dev_idx < 0 && info.input_count &&
                info.output_count)
        {
            /* Set default capture and playback device */
            drv->dev_idx = i;
        }

        if (drv->play_dev_idx >= 0 && drv->rec_dev_idx >= 0 &&
                drv->dev_idx >= 0)
        {
            /* Done. */
            break;
        }
    }

    /* Register the factory */
    drv->f = f;
    drv->f->sys.drv_idx = drv_idx;
    drv->start_idx = aud_subsys.dev_cnt;
    drv->dev_cnt = dev_cnt;

    /* Register devices to global list */
    for (i=0; i<dev_cnt; ++i) {
        aud_subsys.dev_list[aud_subsys.dev_cnt++] = MAKE_DEV_ID(drv_idx, i);
    }

    return PJ_SUCCESS;
}

/* API: deinit driver */
PJ_DEF(void) pjmedia_aud_driver_deinit(unsigned drv_idx)
{
    pjmedia_aud_driver *drv = &aud_subsys.drv[drv_idx];

    if (drv->f) {
        drv->f->op->destroy(drv->f);
        drv->f = NULL;
    }

    pj_bzero(drv, sizeof(*drv));
    drv->play_dev_idx = drv->rec_dev_idx =
                            drv->dev_idx = PJMEDIA_AUD_INVALID_DEV;
}




/*  工厂操作接口函数声明 */

typedef struct pjmedia_aud_dev_factory_op {
    /**
     * Initialize the audio device factory.
     *
     * @param f		The audio device factory.
     */
    pj_status_t (*init)(pjmedia_aud_dev_factory *f);  //pjmedia_aud_dev_factory_op通过pjmedia_aud_dev_factory在不同函数间传递数据

    /**
     * Close this audio device factory and release all resources back to the
     * operating system.
     *
     * @param f		The audio device factory.
     */
    pj_status_t (*destroy)(pjmedia_aud_dev_factory *f);

    /**
     * Get the number of audio devices installed in the system.
     *
     * @param f		The audio device factory.
     */
    unsigned (*get_dev_count)(pjmedia_aud_dev_factory *f);

    /**
     * Get the audio device information and capabilities.
     *
     * @param f		The audio device factory.
     * @param index	Device index.
     * @param info	The audio device information structure which will be
     *			initialized by this function once it returns
     *			successfully.
     */
    pj_status_t	(*get_dev_info)(pjmedia_aud_dev_factory *f,
                                unsigned index,
                                pjmedia_aud_dev_info *info);

    /**
     * Initialize the specified audio device parameter with the default
     * values for the specified device.
     *
     * @param f		The audio device factory.
     * @param index	Device index.
     * @param param	The audio device parameter.
     */
    pj_status_t (*default_param)(pjmedia_aud_dev_factory *f,
                                 unsigned index,
                                 pjmedia_aud_param *param);

    /**
     * Open the audio device and create audio stream. See
     * #pjmedia_aud_stream_create()
     */
    pj_status_t (*create_stream)(pjmedia_aud_dev_factory *f,
                                 const pjmedia_aud_param *param,
                                 pjmedia_aud_rec_cb rec_cb,
                                 pjmedia_aud_play_cb play_cb,
                                 void *user_data,
                                 pjmedia_aud_stream **p_aud_strm);

    /**
     * Refresh the list of audio devices installed in the system.
     *
     * @param f		The audio device factory.
     */
    pj_status_t (*refresh)(pjmedia_aud_dev_factory *f);

}


struct pjmedia_aud_dev_factory
{
    /** Internal data to be initialized by audio subsystem. */
    struct {
        /** Driver index */
        unsigned drv_idx;
    } sys;

    /** Operations */
    pjmedia_aud_dev_factory_op *op;   //是一个虚拟函数指针，和C++中的虚表中却是十分相像
};

/* typedef for factory creation function */
typedef pjmedia_aud_dev_factory*
(*pjmedia_aud_dev_factory_create_func_ptr)(pj_pool_factory*);


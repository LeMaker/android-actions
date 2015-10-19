extern int demux_plugin_info;

void *get_plugin_info(void)
{
    return (void *)&demux_plugin_info;
}

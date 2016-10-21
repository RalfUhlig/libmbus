//------------------------------------------------------------------------------
// Copyright (C) 2010, Raditex AB
// All rights reserved.
//
// rSCADA
// http://www.rSCADA.se
// info@rscada.se
//
//------------------------------------------------------------------------------

#include <err.h>
#include <stdio.h>
#include <string.h>

#include <mbus/mbus.h>

int
main(int argc, char *argv[])
{
    FILE *fp = NULL;
    size_t buff_len, len, i;
    int normalized = 0, json = 0, influxdb = 0;
    unsigned char buf[1024];
    mbus_frame reply;
    mbus_frame_data frame_data;
    char *resultstr = NULL, *file = NULL;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-n") == 0)
        {
            normalized = 1;
        }
        else if (strcmp(argv[i], "-j") == 0)
        {
            json = 1;
            influxdb = 0;
        }
        else if (strcmp(argv[i], "-i") == 0)
        {
            influxdb = 1;
            json = 0;
        }
        else
        {
            file = argv[i];
        }
    }

    if (file == NULL)
    {
        fprintf(stderr, "usage: %s [-n] [-j|-i] binary-file\n", argv[0]);
        fprintf(stderr, "    optional flag -n for normalized values\n");
        fprintf(stderr, "    optional flag -j for result as json string\n");
        fprintf(stderr, "    optional flag -i for result as InfluxDB Line Protocol string\n");
        return 1;
    }

    if ((fp = fopen(file, "r")) == NULL)
    {
        fprintf(stderr, "%s: failed to open '%s'\n", argv[0], file);
        return 1;
    }

    memset(buf, 0, sizeof(buf));
    len = fread(buf, 1, sizeof(buf), fp);

    if (ferror(fp) != 0)
    {
        fprintf(stderr, "%s: failed to read '%s'\n", argv[0], file);
        return 1;
    }

    fclose(fp);

    memset(&reply, 0, sizeof(reply));
    memset(&frame_data, 0, sizeof(frame_data));
    mbus_parse(&reply, buf, len);
    mbus_frame_data_parse(&reply, &frame_data);
    mbus_frame_print(&reply);

    if (influxdb)
    {
        resultstr = normalized ? mbus_frame_data_influxdb_normalized(&frame_data) : mbus_frame_data_influxdb(&frame_data);

        if (resultstr == NULL)
        {
            fprintf(stderr, "Failed to generate InfluxDB Line Protocol representation of MBUS frame: %s\n", mbus_error_str());
            return 1;
        }
        printf("%s", resultstr);
        free(resultstr);
    }
    else if (json)
    {
        resultstr = normalized ? mbus_frame_data_json_normalized(&frame_data) : mbus_frame_data_json(&frame_data);

        if (resultstr == NULL)
        {
            fprintf(stderr, "Failed to generate JSON representation of MBUS frame: %s\n", mbus_error_str());
            return 1;
        }
        printf("%s", resultstr);
        free(resultstr);
    }
    else
    {
        resultstr = normalized ? mbus_frame_data_xml_normalized(&frame_data) : mbus_frame_data_xml(&frame_data);

        if (resultstr == NULL)
        {
            fprintf(stderr, "Failed to generate XML representation of MBUS frame: %s\n", mbus_error_str());
            return 1;
        }
        printf("%s", resultstr);
        free(resultstr);
    }

    return 0;
}

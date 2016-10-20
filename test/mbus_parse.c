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
    int normalized = 0, json = 0;
    unsigned char buf[1024];
    mbus_frame reply;
    mbus_frame_data frame_data;
    char *xml_result = NULL, *json_result = NULL, *file = NULL;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-n") == 0)
        {
            normalized = 1;
        }
        else if (strcmp(argv[i], "-j") == 0)
        {
            json = 1;
        }
        else
        {
            file = argv[i];
        }
    }

    if (file == NULL)
    {
        fprintf(stderr, "usage: %s [-n] [-j] binary-file\n", argv[0]);
        fprintf(stderr, "    optional flag -n for normalized values\n");
        fprintf(stderr, "    optional flag -j for result as json string instead of xml string\n");
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

    if (json)
    {
        json_result = normalized ? mbus_frame_data_json_normalized(&frame_data) : mbus_frame_data_json(&frame_data);

        if (json_result == NULL)
        {
            fprintf(stderr, "Failed to generate JSON representation of MBUS frame: %s\n", mbus_error_str());
            return 1;
        }
        printf("%s", json_result);
        free(json_result);
    }
    else
    {
        xml_result = normalized ? mbus_frame_data_xml_normalized(&frame_data) : mbus_frame_data_xml(&frame_data);

        if (xml_result == NULL)
        {
            fprintf(stderr, "Failed to generate XML representation of MBUS frame: %s\n", mbus_error_str());
            return 1;
        }
        printf("%s", xml_result);
        free(xml_result);
    }

    return 0;
}

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
    size_t buff_len, len;
    int result, normalized = 0, json = 0;
    unsigned char raw_buff[4096], buff[4096];
    mbus_frame reply;
    mbus_frame_data frame_data;
    char *str_result = NULL, *file = NULL;
    size_t i;

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
        fprintf(stderr, "usage: %s [-n] [-j] hex-file\n", argv[0]);
        fprintf(stderr, "    optional flag -n for normalized values\n");
        fprintf(stderr, "    optional flag -j for result as json string instead of xml string\n");
        return 1;
    }

    if ((fp = fopen(file, "r")) == NULL)
    {
        fprintf(stderr, "%s: failed to open '%s'\n", argv[0], file);
        return 1;
    }

    memset(raw_buff, 0, sizeof(raw_buff));
    len = fread(raw_buff, 1, sizeof(raw_buff), fp);

    if (ferror(fp) != 0)
    {
        fprintf(stderr, "%s: failed to read '%s'\n", argv[0], file);
        return 1;
    }

    fclose(fp);

    buff_len = mbus_hex2bin(buff,sizeof(buff),raw_buff,sizeof(raw_buff));

    memset(&reply, 0, sizeof(reply));
    memset(&frame_data, 0, sizeof(frame_data));

    //mbus_parse_set_debug(1);

    result = mbus_parse(&reply, buff, buff_len);

    if (result < 0)
    {
        fprintf(stderr, "mbus_parse: %s\n", mbus_error_str());
        return 1;
    }
    else if (result > 0)
    {
        fprintf(stderr, "mbus_parse: need %d more bytes\n", result);
        return 1;
    }

    result = mbus_frame_data_parse(&reply, &frame_data);

    if (result != 0)
    {
        mbus_frame_print(&reply);
        fprintf(stderr, "mbus_frame_data_parse: %s\n", mbus_error_str());
        return 1;
    }

    //mbus_frame_print(&reply);
    //mbus_frame_data_print(&frame_data);

    if (json)
    {
        str_result = normalized ? mbus_frame_data_json_normalized(&frame_data) : mbus_frame_data_json(&frame_data);

        if (str_result == NULL)
        {
            fprintf(stderr, "Failed to generate JSON representation of MBUS frame: %s\n", mbus_error_str());
            return 1;
        }
        printf("%s", str_result);
        free(str_result);
    }
    else
    {
        str_result = normalized ? mbus_frame_data_xml_normalized(&frame_data) : mbus_frame_data_xml(&frame_data);

        if (str_result == NULL)
        {
            fprintf(stderr, "Failed to generate XML representation of MBUS frame: %s\n", mbus_error_str());
            return 1;
        }
        printf("%s", str_result);
        free(str_result);
    }

    mbus_data_record_free(frame_data.data_var.record);

    return 0;
}


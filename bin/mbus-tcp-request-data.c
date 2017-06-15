//------------------------------------------------------------------------------
// Copyright (C) 2011, Robert Johansson, Raditex AB
// All rights reserved.
//
// rSCADA
// http://www.rSCADA.se
// info@rscada.se
//
//------------------------------------------------------------------------------

#include <string.h>

#include <stdio.h>
#include <mbus/mbus.h>

static int debug = 0;

//------------------------------------------------------------------------------
// Execution starts here:
//------------------------------------------------------------------------------
int
main(int argc, char **argv)
{
    mbus_frame reply;
    mbus_frame_data reply_data;
    mbus_handle *handle = NULL;

    char *host = NULL, *addr_str = NULL, matching_addr[16], *result_str;
    int address;
    long port = 0;
    int json = 0, influxdb = 0, options = 0;
    size_t i = 0;

    memset((void *)&reply, 0, sizeof(mbus_frame));
    memset((void *)&reply_data, 0, sizeof(mbus_frame_data));

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0)
        {
            debug = 1;
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
        else if (strcmp(argv[i], "--onlynumval") == 0)
        {
          options = options | MBUS_VALUE_OPTION_ONLYNUMERIC;
        }
        else if (host == NULL)
        {
            host = argv[i];
        }
        else if (port == 0)
        {
            port = atol(argv[i]);
        }
        else if (addr_str == NULL)
        {
            addr_str = argv[i];
        }
    }

    if (host == NULL || port == 0 || addr_str == NULL)
    {
        fprintf(stderr, "usage: %s [-d] [-j|-i] [--onlynumval] host port mbus-address\n", argv[0]);
        fprintf(stderr, "    optional flag -d for debug printout\n");
        fprintf(stderr, "    optional flag -j for json output\n");
        fprintf(stderr, "    optional flag -i for InfluxDB Line Protocol output\n");
        fprintf(stderr, "    optional flag --onlynumval for supressing records with non-numeric values\n");
        return 0;
    }

    if ((port < 0) || (port > 0xFFFF))
    {
        fprintf(stderr, "Invalid port: %ld\n", port);
        return 1;
    }

    if ((handle = mbus_context_tcp(host, port)) == NULL)
    {
        fprintf(stderr, "Could not initialize M-Bus context: %s\n",  mbus_error_str());
        return 1;
    }

    if (debug)
    {
        mbus_register_send_event(handle, &mbus_dump_send_event);
        mbus_register_recv_event(handle, &mbus_dump_recv_event);
    }

    if (mbus_connect(handle) == -1)
    {
        fprintf(stderr, "Failed to setup connection to M-bus gateway\n");
        return 1;
    }

    if (mbus_is_secondary_address(addr_str))
    {
        // secondary addressing

        int ret;

        ret = mbus_select_secondary_address(handle, addr_str);

        if (ret == MBUS_PROBE_COLLISION)
        {
            fprintf(stderr, "%s: Error: The address mask [%s] matches more than one device.\n", __PRETTY_FUNCTION__, addr_str);
            return 1;
        }
        else if (ret == MBUS_PROBE_NOTHING)
        {
            fprintf(stderr, "%s: Error: The selected secondary address does not match any device [%s].\n", __PRETTY_FUNCTION__, addr_str);
            return 1;
        }
        else if (ret == MBUS_PROBE_ERROR)
        {
            fprintf(stderr, "%s: Error: Failed to select secondary address [%s].\n", __PRETTY_FUNCTION__, addr_str);
            return 1;
        }
        // else MBUS_PROBE_SINGLE

        if (mbus_send_request_frame(handle, MBUS_ADDRESS_NETWORK_LAYER) == -1)
        {
            fprintf(stderr, "Failed to send M-Bus request frame.\n");
            return 1;
        }
    }
    else
    {
        // primary addressing

        address = atoi(addr_str);
        if (mbus_send_request_frame(handle, address) == -1)
        {
            fprintf(stderr, "Failed to send M-Bus request frame.\n");
            return 1;
        }
    }

    if (mbus_recv_frame(handle, &reply) != MBUS_RECV_RESULT_OK)
    {
        fprintf(stderr, "Failed to receive M-Bus response frame: %s\n", mbus_error_str());
        return 1;
    }

    //
    // parse data and print in XML or JSON or InfluxDB Line Protocol format
    //
    if (debug)
    {
        mbus_frame_print(&reply);
    }

    if (mbus_frame_data_parse(&reply, &reply_data) == -1)
    {
        fprintf(stderr, "M-bus data parse error: %s\n", mbus_error_str());
        return 1;
    }

    if (influxdb)
    {
        if ((result_str = mbus_frame_data_influxdb(&reply_data, options)) == NULL)
        {
            fprintf(stderr, "Failed to generate InfluxDB Line Protocol representation of MBUS frame: %s\n", mbus_error_str());
            return 1;
        }

        printf("%s", result_str);
        free(result_str);
    }
    else if (json)
    {
        if ((result_str = mbus_frame_data_json(&reply_data, options)) == NULL)
        {
            fprintf(stderr, "Failed to generate JSON representation of MBUS frame: %s\n", mbus_error_str());
            return 1;
        }

        printf("%s", result_str);
        free(result_str);
    }
    else
    {
        if ((result_str = mbus_frame_data_xml(&reply_data, options)) == NULL)
        {
            fprintf(stderr, "Failed to generate XML representation of MBUS frame: %s\n", mbus_error_str());
            return 1;
        }

        printf("%s", result_str);
        free(result_str);
    }

    // manual free
    if (reply_data.data_var.record)
    {
        mbus_data_record_free(reply_data.data_var.record); // free's up the whole list
    }

    mbus_disconnect(handle);
    mbus_context_free(handle);
    return 0;
}




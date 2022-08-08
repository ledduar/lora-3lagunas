#include "loratap.h"
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <json-c/json.h>
#include <stdlib.h>

#define LINKTYPE_LORA_LORATAP 270

const char *estacion = "tx";

#define puntos (2)
int puntos_array[] = {1, 2};

#define muestras (3)
int muestras_array[] = {1, 2, 3};

#define filas (24)
#define columnas (2)
int sf_array[] = {7, 8, 9, 10, 11, 12};
int cr_array[] = {5, 6, 7, 8};

char json_name[20];
char pcap_name[20];
int int_sf;
int int_cr;
int int_punto;
int int_muestra;

typedef struct pcap_hdr_s
{
    uint32_t magic_number;  /* magic number */
    uint16_t version_major; /* major version number */
    uint16_t version_minor; /* minor version number */
    int32_t thiszone;       /* GMT to local correction */
    uint32_t sigfigs;       /* accuracy of timestamps */
    uint32_t snaplen;       /* max length of captured packets, in octets */
    uint32_t network;       /* data link type */
} pcap_hdr_t;

typedef struct pcaprec_hdr_s
{
    uint32_t ts_sec;   /* timestamp seconds */
    uint32_t ts_usec;  /* timestamp microseconds */
    uint32_t incl_len; /* number of octets of packet saved in file */
    uint32_t orig_len; /* actual length of packet */
} pcaprec_hdr_t;

void dumpLoraTapHeader(loratap_header_t header)
{
    printf("LoRaTap packet header\n");
    printf("Packet RSSI: %d\n", header.rssi.packet_rssi);
    printf("RSSI: %d\n", header.rssi.current_rssi);
    printf("SNR: %d\n", header.rssi.snr);
    printf("Frequency: %u\n", ntohl(header.channel.frequency));
    printf("SF: %2u\n", header.channel.sf);
}

int main()
{

    FILE *captureFile;
    pcap_hdr_t file_header;

    FILE *fp;
    char buffer[30000];
    struct json_object *parsed_json;
    struct json_object *data;
    struct json_object *payload;
    struct json_object *sec;
    struct json_object *rssi;
    struct json_object *snr;
    struct json_object *freq;
    struct json_object *sf;
    struct json_object *bw;
    struct json_object *muestra;
    size_t n_muestras;
    uint16_t bandwidth;

    int array_comb[filas][columnas];
    int size_sf_array = sizeof(sf_array) / sizeof(sf_array[0]);
    int size_cr_array = sizeof(cr_array) / sizeof(cr_array[0]);
    int contador_array = 0;

    for (int i = 0; i < size_cr_array; i++)
    {
        for (int j = 0; j < size_sf_array; j++)
        {
            array_comb[contador_array][0] = sf_array[j];
            array_comb[contador_array][1] = cr_array[i];
            contador_array++;
        }
    }

    size_t j;
    size_t p;
    size_t m;

    for (j = 0; j < filas; j++)
    {
        for (p = 0; p < puntos; p++)
        {
            for (m = 0; m < muestras; m++)
            {  
                const char *file_sf = "s";
                const char *file_cr = "c";
                const char *ext_json = ".json";
                const char *ext_pcap = ".pcap";

                int_sf = array_comb[j][0];
                char char_sf[10];
                sprintf(char_sf, "%d", int_sf);

                int_cr = array_comb[j][1];
                char char_cr[10];
                sprintf(char_cr, "%d", int_cr);

                int_punto = puntos_array[p];
                char char_punto[10];
                sprintf(char_punto, "%d", int_punto);

                int_muestra = muestras_array[m];
                char char_muestra[10];
                sprintf(char_muestra, "%d", int_muestra);

                strcpy(json_name, estacion);
                strcat(json_name, "/p");
                strcat(json_name, char_punto);
                strcat(json_name, "/");
                strcat(json_name, "m");
                strcat(json_name, char_muestra);
                strcat(json_name, file_sf);
                strcat(json_name, char_sf);
                strcat(json_name, file_cr);
                strcat(json_name, char_cr);
                strcat(json_name, ext_json);
                printf("json_name: %s\n", json_name);

                strcpy(pcap_name, "pcap/");
                strcat(pcap_name, estacion);
                strcat(pcap_name, "-p");
                strcat(pcap_name, char_punto);
                strcat(pcap_name, "-");
                strcat(pcap_name, "m");
                strcat(pcap_name, char_muestra);
                strcat(pcap_name, file_sf);
                strcat(pcap_name, char_sf);
                strcat(pcap_name, file_cr);
                strcat(pcap_name, char_cr);
                strcat(pcap_name, ext_pcap);
                printf("pcap_name: %s\n", pcap_name);

                /* Create pcap with header */
                captureFile = fopen(pcap_name, "w");
                file_header.magic_number = 0xa1b2c3d4;
                file_header.version_major = 2;
                file_header.version_minor = 4;
                file_header.thiszone = 0;
                file_header.sigfigs = 0;
                file_header.snaplen = 65535;
                file_header.network = LINKTYPE_LORA_LORATAP;
                fwrite(&file_header, sizeof(pcap_hdr_t), 1, captureFile);
                fclose(captureFile);

                // =====================
                fp = fopen(json_name, "r");
                fread(buffer, 30000, 1, fp);
                fclose(fp);

                parsed_json = json_tokener_parse(buffer);

                json_object_object_get_ex(parsed_json, "data", &data);
                json_object_object_get_ex(parsed_json, "muestra", &muestra);

                n_muestras = json_object_array_length(data);
                printf("Found %lu data\n", n_muestras);

                FILE *captureArch;
                loratap_header_t loratap_packet_header;
                pcaprec_hdr_t pcap_packet_header;

                size_t i;

                for (i = 0; i < n_muestras; i++)
                {
                    muestra = json_object_array_get_idx(data, i);
                    json_object_object_get_ex(muestra, "payload", &payload);
                    json_object_object_get_ex(muestra, "sec", &sec);
                    json_object_object_get_ex(muestra, "rssi", &rssi);
                    json_object_object_get_ex(muestra, "snr", &snr);
                    json_object_object_get_ex(muestra, "freq", &freq);
                    json_object_object_get_ex(muestra, "sf", &sf);

                    /* Create pcap with header */
                    captureArch = fopen(pcap_name, "a");

                    /* Write packet */
                    pcap_packet_header.ts_sec = json_object_get_int(sec);
                    pcap_packet_header.ts_usec = 0;
                    pcap_packet_header.incl_len = strlen(json_object_get_string(payload)) + sizeof(loratap_header_t);
                    pcap_packet_header.orig_len = strlen(json_object_get_string(payload)) + sizeof(loratap_header_t);
                    loratap_packet_header.lt_version = 0;
                    loratap_packet_header.lt_length = htons(sizeof(loratap_header_t));
                    loratap_packet_header.channel.frequency = htonl(json_object_get_int(freq));
                    loratap_packet_header.channel.bandwidth = 1;
                    loratap_packet_header.channel.sf = json_object_get_int(sf);
                    loratap_packet_header.rssi.packet_rssi = 139 + json_object_get_int(rssi);
                    loratap_packet_header.rssi.current_rssi = 139 + json_object_get_int(rssi);
                    loratap_packet_header.rssi.max_rssi = 139 + json_object_get_int(rssi);
                    loratap_packet_header.rssi.snr = (~(json_object_get_int(snr) - 1) & 0xFF) << 2;
                    loratap_packet_header.sync_word = 0x34; // LoRaWAN

                    // dumpLoraTapHeader(loratap_packet_header);
                    fwrite(&pcap_packet_header, sizeof(pcaprec_hdr_t), 1, captureArch);
                    fwrite(&loratap_packet_header, sizeof(loratap_header_t), 1, captureArch);
                    fwrite(json_object_get_string(payload), strlen(json_object_get_string(payload)), 1, captureArch);
                    fclose(captureArch);
                }
            }
        }
    }
}
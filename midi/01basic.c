#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/soundcard.h>
#include <unistd.h>


/* Minimal configuration for OSS
 * For real world applications, this structure will probably contain many
 * more fields
 */
typedef struct config
{
  char *device;
  int fd;
} config_t;


enum eventType
{
  NOTE_MASK = 0xF0,
  NOTE_ON = 0x90,
  NOTE_OFF = 0x80,
  CHANNEL_MASK = 0xF,
  CONTROLER_ON = 0xB0,
  META = 0xFF,
};


typedef struct event
{
  uint8_t type;
  union {
    uint8_t note;
    uint8_t controller;
  };
  union {
    uint8_t velocity;
    uint8_t value;
  };
  uint8_t channel;
  uint8_t meta;
  unsigned char *data;
} event_t;


/* Error state is indicated by value=-1 in which case application exits
 * with error
 */
static void checkError(const int value, const char *message)
{
  if (value == -1)
  {
    fprintf(stderr, "OSS error: %s %s\n", message, strerror(errno));
    exit(1);
  }
}


static void print(event_t e)
{
  switch (e.type)
  {
    case NOTE_ON:
    case NOTE_OFF:
    {
      char *typeStr = e.type == NOTE_ON ? "note on" : "note off";
      printf("type = %s\n", typeStr);
      printf("note = %u\n", e.note);
      printf("channel = %u\n", e.channel + 1u);
      printf("velocity = %u\n", e.velocity);
      break;
    }
    case CONTROLER_ON:
    {
      printf("controller = %u\n", e.controller);
      printf("channel = %u\n", e.channel);
      printf("value = %u\n", e.value);
      break;
    }
    case META:
    {
      printf("meta: 0x%x\n", e.meta);
      break;
    }
    default:
    {
      printf("Unkown event type 0x%x!\n", e.type);
    }
  }
}


static void parseData(event_t *e)
{
  e->type = e->data[0] & NOTE_MASK;
  e->channel = e->data[0] & CHANNEL_MASK;
  if (e->type == NOTE_ON || e->type == NOTE_OFF)
  {
    e->note = e->data[1];
    e->velocity = e->data[2];
  }
  else if (e->type == CONTROLER_ON)
  {
    e->controller = e->data[1];
    e->value = e->data[2];
  }
}


static void convertToData(event_t *e)
{
  e->data[0] = e->type | e->channel;
  if (e->type == CONTROLER_ON)
  {
    e->data[1] = e->controller;
    e->data[2] = e->value;
  }
  else
  {
    e->data[1] = e->note;
    e->data[2] = e->velocity;
  }
  e->data[3] = '\0';
}


int main(int argc, char **argv)
{
  config_t config;
  if (argc < 2) { config.device = "/dev/umidi0.0"; }
  else { config.device = argv[1]; }
  event_t event = { .data = malloc(4) };

  /* Open the device for read and write */
  config.fd = open(config.device, O_RDWR);
  checkError(config.fd, "open");

  while(1)
  {
    read(config.fd, event.data, 4);
    parseData(&event);
    print(event);
    printf("\n");
    convertToData(&event);
    write(config.fd, event.data, 4);
  }
  close(config.fd);
  return 0;
}

using System;
using Newtonsoft.Json;

namespace GarageDoor.Particle
{
    public class CoreInfo
    {
        [JsonProperty("last_app")]
        public string last_app { get; set; }
        [JsonProperty("last_heard")]
        public DateTime last_heard { get; set; }
        [JsonProperty("connected")]
        public bool connected { get; set; }
        [JsonProperty("last_handshake_at")]
        public DateTime last_handshake_at { get; set; }
        [JsonProperty("deviceID")]
        public string deviceID { get; set; }
        [JsonProperty("product_id")]
        public string producdt_id { get; set; }
    }
}
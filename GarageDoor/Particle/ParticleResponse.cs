using GarageDoor.JsonObjects;
using Newtonsoft.Json;

namespace GarageDoor.Particle
{
    public class ParticleVariableResponse
    {
        [JsonProperty("cmd")]
        public string cmd { get; set; }
        [JsonProperty("name")]
        public string name { get; set; }
        [JsonProperty("error")]
        public string error { get; set; }
        [JsonProperty("result")]
        public string result { get; set; }
        [JsonProperty("coreInfo")]
        public CoreInfo coreInfo { get; set; }
    }
}
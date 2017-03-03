using System;
using System.Collections.Generic;
using Newtonsoft.Json;

namespace GarageDoor.JsonObjects
{
    /// <summary>
    /// Mapping minimal json element names with nicer and bigger property names.
    /// We need minimal json due to help manage small payload sizes that can be 
    /// sent back from Particle cloud. 
    /// </summary>
    public class SensorReadingPayload
    {
        [JsonProperty("result")]
        public SensorReadingsForOperation Result { get; set; }
    }

    public class SensorReadingsForOperation
    {
        [JsonProperty("all")]
        public List<SensorReading> Readings { get; set; }
    }

    public class SensorReading
    {
        [JsonProperty("t")]
        public TimeSpan LogTime { get; set; }
        [JsonProperty("r")]
        public int Rpm{ get; set; }
    }
}
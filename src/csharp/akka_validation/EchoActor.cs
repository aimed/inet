using System;
using System.Xml.Linq;
using Akka.Actor;
using SampleApp;

namespace PhyNetFlow.OMNeT
{
    public class EchoActor : UntypedActor, ILogReceive
    {
        private readonly bool _shouldIgnore;
        private readonly bool _isBroadcaster;
        private readonly Network _network;
        
        /// <summary>
        /// Message to be send.
        /// </summary>
        public class Echo
        {
        }
        
        public class EchoBroadcast
        {
        }
        
        public EchoActor(bool shouldIgnore, bool isBroadcaster)
        {
            _shouldIgnore = shouldIgnore;
            _isBroadcaster = isBroadcaster;
            _network = Context.System.GetNetwork();
            
            if (!isBroadcaster)
            {
                _network.Subscribe(Self, typeof(Echo));
            }
        }

        protected override void OnReceive(object message)
        {
            Console.WriteLine("Received a message.");
            if (message is EchoBroadcast)
            {
                _network.Broadcast(Self, new Echo());
                
                // After 1 second broadcast new echo message.
                Context.System.Scheduler.ScheduleTellOnce(TimeSpan.FromMilliseconds(1000), Self, new EchoBroadcast(), Self);
                return;
            }
            
            if (message is Echo && !_shouldIgnore)
            {
                _network.Send(Self, Sender, message);
            }
        }

        public static Props Props(bool shouldIgnore = false, bool isBroadcaster = false)
        {
            return Akka.Actor.Props.Create(() => new EchoActor(shouldIgnore, isBroadcaster));
        }
    }
}

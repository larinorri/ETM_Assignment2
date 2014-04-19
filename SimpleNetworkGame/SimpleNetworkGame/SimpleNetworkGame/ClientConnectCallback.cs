using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Threading;

using PhoneDirect3DXamlAppInterop;
using PhoneDirect3DXamlAppComponent;

namespace ClientConnectCallback
{
    // derived of native class which can submit messages to this class 
    public class CClientConnectCallback : IClientConnectedCallback
    {
        Dispatcher _d;
        MainPage GUI;
        //TextBlock _txtReceived;

        public void Init(Dispatcher d, MainPage p)//TextBlock t)
        {
            _d = d;
            GUI = p;
            //_txtReceived = t;
        }

        public void ClientConnectedServer(string ipAddressOfClient)
        {
            // weird syntax... allows async notiifcation of GUI thread 
            _d.BeginInvoke(() =>
            {
                //_txtReceived.Text = ipAddressOfClient;
                GUI.OnNotifyClientConnection(ipAddressOfClient);
            });

        }
    }
}

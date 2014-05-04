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

namespace GameEndedCallback
{
    // derived of native class which can submit messages to this class 
    public class CGameEndedCallback : IGameEndedCallback
    {
        Dispatcher _d;
        MainPage GUI;
       
        public void Init(Dispatcher d, MainPage p)
        {
            _d = d;
            GUI = p;
        }

        public void GameOver(string winnar)
        {
            // weird syntax... allows async notiifcation of GUI thread 
            _d.BeginInvoke(() =>
            {
                GUI.OnNotifyGameOver(winnar);
            });

        }
    }
}


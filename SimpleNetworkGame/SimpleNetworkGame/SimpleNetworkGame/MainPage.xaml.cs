using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Shapes;
using Microsoft.Phone.Controls;

// ALL C++/CX classes MUST MUST MUST be in this namespace!!!!!!
using PhoneDirect3DXamlAppComponent;
using ClientConnectCallback;

// My netwoking communication classes
//using GameClient; // STUPID!!! why didnt they tell me not to use my own namespaces!!!
// The tech name of this stuff is C++/CX

namespace PhoneDirect3DXamlAppInterop
{
    public partial class MainPage : PhoneApplicationPage
    {
        private Direct3DBackground m_d3dBackground = null;

        // Constructor
        public MainPage()
        {
            InitializeComponent();
            // hide this item by default
            this.HostIPTextBlock.Visibility = System.Windows.Visibility.Collapsed;
            this.StatusTextBlock.Visibility = System.Windows.Visibility.Collapsed;
        }

        private void DrawingSurfaceBackground_Loaded(object sender, RoutedEventArgs e)
        {
            if (m_d3dBackground == null)
            {
                m_d3dBackground = new Direct3DBackground();

                // Set window bounds in dips
                m_d3dBackground.WindowBounds = new Windows.Foundation.Size(
                    (float)Application.Current.Host.Content.ActualWidth,
                    (float)Application.Current.Host.Content.ActualHeight
                    );

                // Set native resolution in pixels
                m_d3dBackground.NativeResolution = new Windows.Foundation.Size(
                    (float)Math.Floor(Application.Current.Host.Content.ActualWidth * Application.Current.Host.Content.ScaleFactor / 100.0f + 0.5f),
                    (float)Math.Floor(Application.Current.Host.Content.ActualHeight * Application.Current.Host.Content.ScaleFactor / 100.0f + 0.5f)
                    );

                // Set render resolution to the full native resolution
                m_d3dBackground.RenderResolution = m_d3dBackground.NativeResolution;

                // Hook-up native component to DrawingSurfaceBackgroundGrid
                DrawingSurfaceBackground.SetBackgroundContentProvider(m_d3dBackground.CreateContentProvider());
                DrawingSurfaceBackground.SetBackgroundManipulationHandler(m_d3dBackground);
            }
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {

        }

        private void OnJoinGame(object sender, RoutedEventArgs e)
        {
            // attempt to connect to game server...
            string error = "";
            var client = new GameClient();
            client.ConnectThroughSocket(this.ServerIPField.Text, out error);
            
            if (error != "")
            {
                MessageBox.Show("Error: " + error);
                // if failed... say so
                this.StatusTextBlock.Visibility = System.Windows.Visibility.Visible;
            }
            else
            {
                // if successfull, hide all interface buttons
                this.ServerIPField.Visibility = System.Windows.Visibility.Collapsed;
                this.StatusTextBlock.Visibility = System.Windows.Visibility.Collapsed;
                this.JoinButton.Visibility = System.Windows.Visibility.Collapsed;
                this.HostButton.Visibility = System.Windows.Visibility.Collapsed;
                this.HostIPTextBlock.Visibility = System.Windows.Visibility.Collapsed;

                // begin the game as the CLIENT
                this.m_d3dBackground.StartGame(false);
            }
            

        }

        private void Button_Click_1(object sender, RoutedEventArgs e)
        {

        }

        private void OnHostGame(object sender, RoutedEventArgs e)
        {
            // launch the game server, wait for a client to connect
            string ip;
            string error;

            var callbackObject = new CClientConnectCallback();
            callbackObject.Init(Dispatcher, this);

            var winsockListener = new GameServer();

            //setup the callback object so the winsock thread can tell us when it gets connections
            winsockListener.SetClientCallBack(callbackObject);

            //start the server listening
            winsockListener.StartSocketServer(out ip, out error);
            if (error != "")
                MessageBox.Show("Error:" + error);
            else
            {
                //server ip address
                this.HostIPTextBlock.Text = "Host IP Address: " + ip + "\nWaiting for Player 2...";

                // hide the standard interface show the IP to join
                this.ServerIPField.Visibility = System.Windows.Visibility.Collapsed;
                this.StatusTextBlock.Visibility = System.Windows.Visibility.Collapsed;
                this.JoinButton.Visibility = System.Windows.Visibility.Collapsed;

                this.HostButton.IsEnabled = false;
                this.HostIPTextBlock.Visibility = System.Windows.Visibility.Visible;
            }
          
        }

        private void ServerIPChanged(object sender, TextChangedEventArgs e)
        {
            this.StatusTextBlock.Visibility = System.Windows.Visibility.Collapsed;
        }
        
        // called by Client Connect Call back on a client connection
        public void OnNotifyClientConnection(string ipAddress)
        {
            // hide all GUI interfaces & begin the game
            this.ServerIPField.Visibility = System.Windows.Visibility.Collapsed;
            this.StatusTextBlock.Visibility = System.Windows.Visibility.Collapsed;
            this.JoinButton.Visibility = System.Windows.Visibility.Collapsed;
            this.HostButton.Visibility = System.Windows.Visibility.Collapsed;
            this.HostIPTextBlock.Visibility = System.Windows.Visibility.Collapsed;

            // begin the game as the HOST
            this.m_d3dBackground.StartGame(true);
        }
    }
}

/******************************************************************************
 * This is the client side of the Space Shooter project for CISS465
 *****************************************************************************/


/******************************************************************************
 * BEGIN Includes.
 *****************************************************************************/

// Standard includes

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// SDL wrapper from Dr. Liow

#include "Includes.h"
#include "Event.h"
#include "compgeom.h"
#include "Constants.h"
#include "Surface.h"



// SDL net
#include "SDL_net.h"


/******************************************************************************
 * END Includes.
 *****************************************************************************/


/******************************************************************************
 * Class definitions.
 *****************************************************************************/
class Player 
{
public:
	Player(int x1, int y1, int num = -1, int state = 0)
		: x(x1), y(y1), size(10), number(num), status(state)
	{
        switch(num)
        {
           case 0:
               color[0] = 255;
               color[1] = 0;
               color[2] = 0;
               break;

            case 1:
                color[0] = 0;
                color[1] = 255;
                color[2] = 0;
                break;

            case 2:
                color[0] = 0;
                color[1] = 0;
                color[2] = 255;
                break;
                
            default:
                color[0] = 255 / 10;
                color[1] = 255 / 5;
                color[2] = 255 / 3;
        }
	}
        
    int x, y, size;
	int number, status;
    int color[3];
    Rect rect;
};


/******************************************************************************
 * Global Constants.
 *****************************************************************************/
const int MAXLEN = 1024;


/******************************************************************************
 * Global Variables.
 *****************************************************************************/
std::vector<Player> players;
std::vector <Rect> alien;

// std::vector<Player> rect;
SDL_Thread *net_thread = NULL, *local_thread = NULL;
int player_number = -1;


/******************************************************************************
 * Functions
 *****************************************************************************/
// Receive a string over TCP/IP
std::string recv_message(TCPsocket sock)
{
    char buff[MAXLEN] = {' '};
    SDLNet_TCP_Recv(sock, buff, MAXLEN);

    if (buff == NULL)
    {
        std::string ret = "";
        return ret;
    }
    
    std::string ret(buff, strlen(buff));
    return ret;
}


// Send a string over TCP/IP
int send_message(std::string msg, TCPsocket sock)
{
    char * buff = (char *)msg.c_str();      
    SDLNet_TCP_Send(sock, buff, MAXLEN);

    return 1;
}


void parse_player_data(std::string message)
{
    std::stringstream message_stream(message);

    int num_players = 0, _status = 0;
    int _x, _y;
    
    message_stream >> num_players;
    
	for (int i = 0; i < num_players; i++)
    {
        if (i < players.size())
        {
            message_stream >> players[i].x >> players[i].y >> players[i].status;
        }
        
        else
        {
            message_stream >> _x >> _y >> _status;

            Player player(_x, _y, i, _status);
            players.push_back(player);
        }
    }
    for(int i = 0; i < 15; ++i)
    {
        if(i < alien.size())
        {
            message_stream >> alien[i].x >> alien[i].y;
        }
	}		
}


void recv_player_number(std::string message)
{
	int i = 0;
	std::string temp_num = "";

	if (message[0] == 'N')
    {
		i++;
		while (message[i] != ';')
        {
			temp_num += message[i];
			i++;
		}
	}

	player_number = atoi(temp_num.c_str());
}


int main(int argc, char **argv)
{
	IPaddress ip;
	TCPsocket sock;
  
	int numready;
	Uint16 port;
	SDLNet_SocketSet set;	

	std::string name;
	std::string to_server;
	std::string from_server;

   

	/* check our commandline */
	if(argc < 4)
	{
		std::cout << "Usage: " << argv[0] << " host_ip host_port username" << std::endl;
		return 0;
	}
	
	name = argv[3];
	
	/* initialize SDL */
	if(SDL_Init(SDL_INIT_EVERYTHING) == -1)
	{
		std::cout << "SDL_Init ERROR" << std::endl;
		return 0;
	}

	/* initialize SDL_net */
	if(SDLNet_Init() == -1)
	{
		std::cout << "SDLNet_Init ERROR" << std::endl;
		SDL_Quit();
		return 0;
	}

	set = SDLNet_AllocSocketSet(1);
	if(!set)
	{
		std::cout << "SDLNet_AllocSocketSet ERROR" << std::endl;
		SDLNet_Quit();
		SDL_Quit();
		return 0;
	}

	/* get the port from the commandline */
	port = (Uint16)strtol(argv[2],NULL,0);
	
	/* Resolve the argument into an IPaddress type */
	std::cout << "connecting to " << argv[1] << " port " << port << std::endl;

	if(SDLNet_ResolveHost(&ip,argv[1],port) == -1)
	{
		std::cout << "SDLNet_ResolveHost ERROR" << std::endl;
		SDLNet_Quit();
		SDL_Quit();
		return 0;
	}

	/* open the server socket */
	sock = SDLNet_TCP_Open(&ip);
	if(!sock)
	{
		std::cout << "SDLNet_TCP_Open ERROR" << std::endl;
		SDLNet_Quit();
		SDL_Quit();
		return 0;
	}
	
	if(SDLNet_TCP_AddSocket(set,sock) == -1)
	{
		std::cout << "SDLNet_TCP_AddSocket ERROR" << std::endl;
		SDLNet_Quit();
		SDL_Quit();
		return 0;
	}

	send_message(name, sock);

	std::cout << "Logged in as: " << name << std::endl;
	
	recv_player_number(recv_message(sock));

	std::cout << "player num: " << player_number << std::endl;

	//-------------------------------------------------------------------------
	// GAME SEGMENT
	//-------------------------------------------------------------------------
	Surface surface(W, H);
	Event event;

    Image image1("images/galaxian/GalaxianGalaxip.gif");
    Image image2("images/galaxian/GalaxianGalaxip.rotated.gif");
   
    Image red("images/galaxian/GalaxianRedAlien.gif");	// loads alien image
    Image purple("images/galaxian/GalaxianPurpleAlien.gif"); // loads alien image	
    Image blue("images/galaxian/GalaxianAquaAlien.gif");	// loads alien image
    Image flagship("images/galaxian/GalaxianFlagship.gif");	// loads alien image

    bool move = true;
  
    
    for (int i = 0; i < 3; ++ i)
    {
        for (int j = 0; j < 5; j++)
        {
            Rect a(40 + (i * 30)+ (j * 30), 10 +
                   (i * 20), 38, 38, i);    
            alien.push_back(a);
            
        }
        
    }
    
    
	while(1)
	{
        std::cout << "players.size() is " << players.size() << std::endl;
		numready = SDLNet_CheckSockets(set, 100);
        if(numready == -1)
		{
			std::cout << "SDLNet_CheckSockets ERROR" << std::endl;
			break;
		}

		//-------------------------------------------------------------------------------
		// GET DATA FROM SERVER
		//-------------------------------------------------------------------------------
		from_server = "";
		if(numready && SDLNet_SocketReady(sock))
		{
			from_server = recv_message(sock);
            std::cout << from_server << std::endl;

            parse_player_data(from_server);
		}

		if (event.poll() && event.type() == QUIT) break;

		KeyPressed keypressed = get_keypressed();

		to_server = "";
        
	    if (keypressed[LEFTARROW])
        {
			to_server = "1";
			send_message(to_server, sock);
		}
		else if (keypressed[RIGHTARROW])
        {
			to_server = "2";
			send_message(to_server, sock);
		}
   
        
        if(move) // moves aliens right
        {
            
            for (int j = 0; j < alien.size(); j++)
            {
                if (move)
                {
                    alien[j].x += 2; 
                }
            }
            for (int i = 0; i < alien.size(); i++)
            {
                if(alien[i].x == W - 30)
                {
                    move = false;
                }
            }
            
        }
            
        if (!move) // moves aliens left
        {
            
            for (int i = 0; i < alien.size(); i++)
            {
                alien[i].x -= 2; 
            }

            for (int i = 0; i < alien.size(); i++)
            {
                if(alien[i].x == 0)
                {
                    move = true;
                }
            }
                
            
        }

        //========================================
        // COPY VALUES OF PLAYER CLASS TO OUR RECT
        //========================================
        
        
		surface.fill(BLACK);
        
        for (int i = 0; i < players.size(); i++)
        {
            if (players[i].status)
            {
                surface.lock();
              
                players[i].rect.x = players[i].x;
                players[i].rect.y = players[i].y;
                if (i == 0)
                {
                    surface.put_image(image1, players[i].rect);
                }
                if (i == 1)
                {
                    surface.put_image(image2, players[i].rect);
                }
                
                surface.unlock();
            }
        }
        for (int i = 0; i < alien.size(); ++i)
        {
            surface.lock();
            
            if(alien[i].color < 1) 
            {
                surface.put_image(flagship, alien[i]);
            }
            
            if(alien[i].color >= 1 && alien[i].color < 2) 
            {
                surface.put_image(red, alien[i]);
            }
            
            if(alien[i].color >= 2 && alien[i].color < 3) 
            {
                surface.put_image(purple, alien[i]);
            }
            
            if(alien[i].color >= 3) 
            {
                surface.put_image(blue, alien[i]);
            }
            
            surface.unlock();
            
        }
        

		surface.flip();
        

		delay(1);
	}

	SDLNet_Quit();
	SDL_Quit();
    
	return(0);
}




//==========================================
// SCRATCH WORK ZONE...
// please do not touch except when necessary
//==========================================

//================================================================
// Small Player Class written by Rotshak for testing purposes only 
/*
class Player
{
public:
    Player(int x1, int y1, int num = -1, int state = 0)
        :x(x1), y(y1), size(10), number(num), status(state)
    {
        switch(num)
        {
            case 0:
                color[0] = 255;
                color[1] = 0;
                color[2] = 0;
                break;

            default:
                color[0] = 255 / 10;
                color[1] = 255 / 5;
                color[2] = 255 / 3;
        }
	}
    
    int x, y, size;
	int number, status;
    int color[3];
    Rect rect;
};
*/

 // CASES 3 to 6...we only need two since its a two player game             
 /*            case 3:
                color[0] = 255;
                color[1] = 255;
                color[2] = 0;
                break;
                
            case 4:
                color[0] = 255;
                color[1] = 0;
                color[2] = 255;
                break;

            case 5:
                color[0] = 0;
                color[1] = 255;
                color[2] = 255;
                break;

            case 6:
                color[0] = 255;
                color[1] = 255;
                color[2] = 255;
                break;
 */

//
//  ViewController.m
//  UDPHolePunching
//
//  Created by Liao KuoHsun on 2014/7/3.
//  Copyright (c) 2014年 Liao KuoHsun. All rights reserved.
//

#import "ViewController.h"
#include "libtwp2p.h"
@interface ViewController ()

@end

@implementation ViewController


- (void)viewDidLoad
{
    [super viewDidLoad];
    
    // TODO:
    int vSocket = 0;
    int vPeerPort=0, vClientListenPort=7272;
    char pPeerAddress[32]={0};
    char pServerAddr[] = "192.168.82.85"; //192.168.82.85, 54.199.200.164

    vSocket = punching("en1", vClientListenPort, pServerAddr, pPeerAddress, &vPeerPort);
    if(vSocket>0)
    {
        printf("Connection to %s:%d is esablised\n",pPeerAddress, vPeerPort);
        close(vSocket);
    }
	// Do any additional setup after loading the view, typically from a nib.
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end

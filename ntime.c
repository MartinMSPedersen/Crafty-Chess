From chessman@voicenet.comSun Sep 29 21:10:27 1996
Date: Sun, 29 Sep 1996 09:16:53 -0400
From: "Michael F. Byrne" <chessman@voicenet.com>
To: Robert Hyatt <hyatt@cis.uab.edu>
Subject: revised time.c

    [The following text is in the "ISO-8859-1" character set]
    [Your display is set for the "US-ASCII" character set]
    [Some characters may be displayed incorrectly]

Hi Bob,

I know you're getting near the end of putting the finishing touches on
crafty ..I believe for the tournament ...since this is a fixed timecontrol
of 30/60; 40/60 there is no need to speed up the program at all if the
opponent is playing.., thus I have added that as a qualifier in time.c
where we accelerate crafty ...also that will take out the potential for
crafty to speedup while the opponent is still in book - I have enclosed the
new time.c ..below is the excerpt of code that changed...

Mike



if (time_used+time_used_opponent < test_time_limit && last_value>0 &&
      time_used+time_used_opponent>=1 && search_type != puzzle &&
      tc_time_remaining<tc_time_remaining_opponent && !auto_kibitzing &&
	  mode!=tournament_mode) {                                /* new coded
added here*/                         
time_limit=Max(time_used+time_used_opponent,1+tc_increment/2);
    Print(1,"time_used   = %s\n",DisplayTime(time_used));
    Print(1,"time_used_opponent = %s\n",DisplayTime(time_used_opponent));
    Print(1,"reduced to time limit %s,  opponent is playing fast\n", 
          DisplayTime(time_limit));
  }
/* auto_kibitzers*/
  if (time_used+time_used_opponent < test_time_limit && last_value>0 &&
      time_used+time_used_opponent>=1 && search_type != puzzle &&
      tc_time_remaining<tc_time_remaining_opponent &&
      auto_kibitzing && mode!=tournament_mode) {                  /* new
coded added here*/  
    time_limit=Max(time_used+time_used_opponent,1+tc_increment/2);
    Print(1,"time_used   = %s\n",DisplayTime(time_used));
    Print(1,"time_used_opponent = %s\n",DisplayTime(time_used_opponent));
    Print(1,"reduced to time limit %s,  computer opponent is playing
fast\n", 
          DisplayTime(time_limit));
  } 
  if (time_limit <= 1) {
    time_limit= 1;
    usage_level=0;
  }
  [Part 2, "attachment; filename="time.zip""  Attached file "time.zip"  5.1KB]
  [Unable to print this part]


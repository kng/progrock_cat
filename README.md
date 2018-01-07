# progrock_cat
Rig control of QRP-Labs.com ProgRock using Arduino

Get/set VFO A and B frequency, sent to the progrock register 4 and 7
Get/set VFO A/B, set digital output to connect to the progrock bank select input
Get rig information, large block of most parameters summarized
Get/set operating mode, no action yet
Get rig ID, should identify as a Kenwood TS-480, easy to change
Get rig name, don't know if this is used at all.
Get rig power status
Get RIT/XIT
Get signal meter

Some of there are just dummies to satisfy the client running the rigcat, these may need to be adjusted as some shold not produce any response.

Some testing with OmniRig and rigctl from hamlib.

rigctl -m 228 -s 9600 -r COM3

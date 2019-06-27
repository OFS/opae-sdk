# $Header: //acds/main/quartus/devices/firmware/tools/sign/arc_resource_publish.pm#3 $
# Definition of ndsign resource
#

package ndsign::__VERSION_UNDERSCORE__;

use DefineResource;

my $ndsign_ver    = '__VERSION__';
my $ndsign_root   = $DefineResource::TOOLS_TAG.'/ndsign/'.$ndsign_ver;
my $ndsign_bin = $ndsign_root;


sub params() {

    return (
       __resource_name => 'ndsign/'.${ndsign_ver},
       description => 'Signing utility for signing nadderd firmware',
       definition_source => '$Header: //acds/main/quartus/devices/firmware/tools/sign/arc_resource_publish.pm#3 $',
       NDSIGN_PATH => "${ndsign_bin}",
       WINDOWS_LOCAL_CSS_PATH => '${APPDATA}/CSS_HOME/Intel/LT\ CSS/Bin'
   );
}
1;

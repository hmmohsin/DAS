set ns [new Simulator]                  
source tb_compat.tcl

set maxServers 10
set maxClients 2
set lanstr ""

for {set i 1} {$i <= $maxServers} {incr i} {
	set S($i) [$ns node]
  	append lanstr "$S($i) "
        tb-set-node-os $S($i) HM-Hadoop-Single
	tb-set-hardware $S($i) d710

	set sbs($i) [$ns blockstore]
	$sbs($i) set-class "local"
	$sbs($i) set-size "100GB"
	$sbs($i) set-placement "sysvol"
	$sbs($i) set-mount-point "/mnt/extra"
	$sbs($i) set-node $S($i)
}

for {set i 1} {$i <= $maxClients} {incr i} {
	set C($i) [$ns node]
  	append lanstr "$C($i) "
	tb-set-node-os $C($i) HM-Hadoop-Single
        tb-set-hardware $C($i) d430

	set cbs($i) [$ns blockstore]
	$cbs($i) set-class "local"
	$cbs($i) set-size "200GB"
	$cbs($i) set-placement "sysvol"
	$cbs($i) set-mount-point "/mnt/extra"
	$cbs($i) set-node $C($i)
}

set lan0 [$ns make-lan "$lanstr" 1Gb 0ms]

for {set i 1} {$i <= $maxClients} {incr i} {

	tb-set-node-lan-bandwidth $C($i) $lan0 10Gb
}

# Go!
$ns run

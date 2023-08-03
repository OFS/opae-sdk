##
## User clock frequency configuration support.
##

##
## For systems where there are two aligned user clocks: one primary and one running
## at half the speed of the primary.
##
## Input: a pair of user clock frequency requests from the AFU's JSON file,
## passed as an array, and the maximum permitted frequency.
##
## Output: target frequencies for the user clocks.
##
## The AFU JSON may say "auto" or "auto-<target freq>" as a user clock frequency.
## In that case, the clock will be constrained in the fitter either to
## "target freq" or to freq_max.  The actual frequencies will be set based on
## the timing results.
##
## Precedence when the high/low clock configurations conflict:
##   1. High clock auto.  The low clock will be set to half the frequency of
##      the high clock.
##   2. Low clock auto.  The high clock will be set to twice the frequency of
##      the low clock.
##   3. Numeric frequencies.  If both clock's frequencies are specified, the
##      low clock must be half the frequency of the high clock.  If only one
##      clock's frequency is specified, the other clock's frequency is derived
##      from the configured clock.
##
proc get_aligned_user_clock_targets {afu_json_uclk_freqs freq_max} {
    set uclk_freq_low [lindex $afu_json_uclk_freqs 0]
    set uclk_freq_high [lindex $afu_json_uclk_freqs 1]

    # First, check whether the user clock frequencies have already been
    # computed following place & route.
    set computed_uclk [load_computed_user_clocks $freq_max]
    if {[llength $computed_uclk]} {
        post_message [concat "Target user clock high: computed " [lindex $computed_uclk 1]]
        post_message [concat "Target user clock low: computed " [lindex $computed_uclk 0]]

        return $computed_uclk
    }

    if {0 == [string compare -nocase -length 4 "auto" $uclk_freq_high]} {
        # High frequency clock is "auto".  Initially, constrain it
        # to a maximum frequency.  Also, ignore the JSON constraint
        # on the low frequency clock and constrain it to half of
        # freq_max.
        set uclk_freq_high [uclk_parse_auto_freq $uclk_freq_high $freq_max]
        set uclk_freq_low [expr {$uclk_freq_high / 2.0}]

        post_message "Target user clock high: auto ($uclk_freq_high)"
        post_message "Target user clock low: auto ($uclk_freq_low)"
    } elseif {0 == [string compare -nocase -length 4 "auto" $uclk_freq_low]} {
        # Low frequency clock is "auto" and the high frequency clock is not.
        # Don't allow a conflicting high frequency constraint.
        if {$uclk_freq_high > 0} {
            error "Fixed user clock high ($uclk_freq_high) and auto user clock low are incompatible."
        }

        set uclk_freq_low [uclk_parse_auto_freq $uclk_freq_low $freq_max]
        set uclk_freq_high [expr {$uclk_freq_low * 2}]
        if {$uclk_freq_high > $freq_max} {
            set uclk_freq_high $freq_max
        }

        post_message "Target user clock high: auto ($uclk_freq_high)"
        post_message "Target user clock low: auto ($uclk_freq_low)"
    } elseif {$uclk_freq_low > 0 || $uclk_freq_high > 0} {
        # At least one clock was specified in the JSON.
        # Were both clocks specified?  If not, complete the settings.
        if {$uclk_freq_low == 0} {set uclk_freq_low [expr {$uclk_freq_high / 2.0}]}
        if {$uclk_freq_high == 0} {set uclk_freq_high [expr {$uclk_freq_low * 2}]}

        # Validate.  Low is supposed to be 1/2 the frequency of high and the two are
        # aligned.  We avoid false negatives caused by floating point rounding here.
        if {[expr {abs((1.0 * $uclk_freq_high) / $uclk_freq_low - 2.0)}] > 0.01} {
            error "Target user clock low ($uclk_freq_low) is supposed to be half the frequency of user clock high ($uclk_freq_high)!"
        }

        # Clamp the clocks at freq_max
        if {$uclk_freq_high > $freq_max} {
            set uclk_freq_high $freq_max
        }
        if {$uclk_freq_low > $freq_max} {
            set uclk_freq_low $freq_max
        }

        post_message "Target user clock high: $uclk_freq_high"
        post_message "Target user clock low: $uclk_freq_low"
    }

    return [list $uclk_freq_low $uclk_freq_high]
}


##
## Parse target frequencies stored in the AFU JSON declared as "auto".
## In auto mode, set the frequency to the maximum possible.  The design
## will be constrained later with the achieved frequency.
##
## When "auto" is followed by a number, e.g. "auto-320", use the number
## as the maximum frequency.
##
proc uclk_parse_auto_freq {req freq_max} {
    set tgt_freq $freq_max

    # We already know that $req begins with "auto".  If there is a
    # dash then the maximum frequency may be specified, too.
    if {"-" == [string index $req 4]} {
        set f [string range $req 5 end]
        if {[string is integer $f] && $f} {
            set tgt_freq $f
        }

        if {$tgt_freq > $freq_max} {
            set tgt_freq $freq_max
        }
    }

    return $tgt_freq
}


##
## Delete the file containing computed user clock frequencies.
##
proc delete_computed_user_clocks_file {} {
    file delete "output_files/user_clock_freq.txt"
}


##
## Save computed user clock frequencies to a file.
##
proc save_computed_user_clocks {uclk_freqs} {
    set uclk_freq_low [lindex $uclk_freqs 0]
    set uclk_freq_high [lindex $uclk_freqs 1]

    post_message "Saved actual user clock high: $uclk_freq_high"
    post_message "Saved actual user clock low: $uclk_freq_low"

    set clkfile [open "output_files/user_clock_freq.txt" w]
    puts $clkfile "# Generated by Platform Interface Manager user_clock_config.tcl"
    puts $clkfile "afu-image/clock-frequency-low:${uclk_freq_low}"
    puts $clkfile "afu-image/clock-frequency-high:${uclk_freq_high}"
    close $clkfile
}


##
## Load user clock frequencies that have already been computed following
## place & route.  The script that computes them stores JSON update
## requests in a file.
##
proc load_computed_user_clocks {freq_max} {
    if {[file exists "output_files/user_clock_freq.txt"]} {
        set fp [open "output_files/user_clock_freq.txt"]

        # Contents of the file
        set v [read $fp]
        # Drop comments
        set v [string trim [regsub -all -line {(^#.*$)} $v ""]]

        # Drop most of the JSON field names, leaving just high:<freq>
        # and low:<freq>.
        set v [regsub -all {afu-image/clock-frequency-} $v ""]
        set uclk_freqs_in [split $v]

        # Sorting puts low/high in proper order
        set uclk_freqs_in [lsort -ascii -decreasing $uclk_freqs_in]

        # Extract just the numbers
        set uclk_freqs [regsub -all {[\w]*:} $uclk_freqs_in ""]

        # Clamp the high frequency clock to freq_max.  It may be out of
        # bounds if only the low frequency clock is used.
        if {[lindex $uclk_freqs 1] > $freq_max} {
            lset uclk_freqs 1 $freq_max
        }

        if {[llength $uclk_freqs] != 2} {
            error "output_files/user_clock_freq.txt should describe two clocks"
        }
        if {[lindex $uclk_freqs 0] > [lindex $uclk_freqs 1]} {
            error "output_files/user_clock_freq.txt slow clock should be first"
        }

        return $uclk_freqs
    }

    # File not found
    return [list]
}


##
## Did the AFU JSON specify auto-frequency?
##
proc uclk_freq_is_auto {afu_json_uclk_freqs} {
    set u_clk_auto 0

    set llen [llength $afu_json_uclk_freqs]
    set i 0
    while {$i < $llen} {
        # Is the requested clock frequency "auto"?
        if {0 == [string compare -nocase -length 4 "auto" [lindex $afu_json_uclk_freqs $i]]} {
            set u_clk_auto 1
        }
        incr i
    }

    return $u_clk_auto
}


##
## Pick frequencies for aligned uClk_usr and uClk_usrDiv2 when given both the
## AFU's JSON constraint and the actual, achieved, frequencies.  This function
## is called after place and route and after the initial timing analysis.
##
proc uclk_pick_aligned_freqs {afu_json_uclk_freqs uclk_freqs_actual freq_max} {
    # Get the user clock targets.  This includes parsing of "auto-<max>", where
    # a maximum frequency is specified in the JSON.  Set FMax to 2 * freq_max in
    # case only the div2 clock is in auto mode, in which case the primary clock
    # is assumed not to be used.  The real FMax will eventually constrain the
    # primary clock if it is used.
    set uclk_tgts [get_aligned_user_clock_targets $afu_json_uclk_freqs [expr {2 * $freq_max}]]

    set uclk_actual_low [lindex $uclk_freqs_actual 0]
    set uclk_actual_high [lindex $uclk_freqs_actual 1]

    set uclk_tgt_low [lindex $uclk_tgts 0]
    set uclk_tgt_high [lindex $uclk_tgts 1]

    if {0 == [string compare -nocase -length 4 "auto" [lindex $afu_json_uclk_freqs 1]]} {

        # User clock high is in auto mode.  Ignore AFU JSON constraints on the low
        # user clock.  The clock is also constrained by the achieved frequency of
        # the low speed user clock.
        post_message "User clock high is auto"
        set uclk_freq_high [::tcl::mathfunc::min \
                                $uclk_actual_high $uclk_tgt_high \
                                [expr {2 * $uclk_actual_low}] \
                                $freq_max]

        # Round down because our user clock constraints support only one decimal
        # point, which might be needed for the div2 clock.
        set uclk_freq_high [expr {int(floor($uclk_freq_high))}]
        set uclk_freq_low [expr ($uclk_freq_high / 2.0)]

    } elseif {0 == [string compare -nocase -length 4 "auto" [lindex $afu_json_uclk_freqs 0]]} {

        # User clock low is in auto mode and high is not.  Assume that the high clock
        # is not used in the design and that user clock low may peak at FMax.
        # This configuration may cause the two clocks to be unaligned, with the
        # high clock hitting a maximum lower than 2x the low speed clock.
        post_message "User clock low is auto and high is not.  Assuming high is unused."
        set uclk_freq_low [::tcl::mathfunc::min $uclk_actual_low $uclk_tgt_low $freq_max]
        set uclk_freq_low [expr {int(floor($uclk_freq_low))}]

        # High clock must be set.  It may be higher than freq_max since it is assumed
        # to be unused.  The user clock logic requires this in order to set the
        # div2 clock correctly.
        set uclk_freq_high [expr ($uclk_freq_low * 2)]

    } else {
        error "uclk_pick_aligned_freqs called but no user clocks are in auto mode."
    }

    return [list $uclk_freq_low $uclk_freq_high]
}

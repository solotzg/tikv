#!/usr/bin/python3
import argparse
import os


def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('--region-split', help='one region id for split')
    parser.add_argument('--region-merge', help=r'two region id for merge, such as "source,target"')

    args = parser.parse_args()
    pd_ctrl_bin = os.getenv("DEFAULT_TEST_PD_CTRL_BIN")
    pd_addr = os.getenv("DEFAULT_TEST_PD_ADDRESS")

    if not pd_addr:
        raise Exception("miss DEFAULT_TEST_PD_ADDRESS")
    if not pd_ctrl_bin:
        raise Exception("miss DEFAULT_TEST_PD_CTRL_BIN")

    cmd = None
    if args.region_split:
        region_id = int(args.region_split)
        cmd = r'{} --pd {} operator add split-region {}'.format(pd_ctrl_bin, pd_addr, region_id)
    elif args.region_merge:
        regions = [int(x.strip()) for x in args.region_merge.split(',')]
        assert len(regions) == 2
        cmd = r'{} --pd {} operator add merge-region {} {}'.format(pd_ctrl_bin, pd_addr, regions[0], regions[1])

    print("cmd is", cmd)
    f = os.popen(cmd, "r")
    print(f.read())
    f.close()


if __name__ == '__main__':
    main()

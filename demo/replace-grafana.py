#!/usr/bin/python3
import argparse
import os


def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('--files', help='input grafana template files for replacing', required=True)
    parser.add_argument('--output', help='dir to output grafana files', required=True)
    parser.add_argument('--deploy-tag', help='generate grafana by tag for deploying in dashboard')
    parser.add_argument('--prometheus_metric_name_prefix', help='prometheus metric name prefix', required=True)
    parser.add_argument('--engine_label_value', help='engine label value', required=True)

    args = parser.parse_args()

    replace_argv = (
        "thread_cpu_seconds_total", "threads_io_bytes_total", "thread_voluntary_context_switches",
        "thread_nonvoluntary_context_switches", "threads_state")
    replace_argv2 = (
        "process_cpu_seconds_total", "process_virtual_memory_bytes", "process_resident_memory_bytes",
        "process_start_time_seconds")

    src_files = None
    if args.files:
        src_files = [x.strip() for x in args.files.split(',')]

    if not src_files:
        raise Exception("wrong grafana template files {}".format(src_files))

    output_dir = args.output

    if not output_dir:
        raise Exception("wrong grafana files output dir {}".format(output_dir))

    os.makedirs(output_dir, exist_ok=True)

    prometheus_metric_name_prefix = args.prometheus_metric_name_prefix
    engine_label_value = args.engine_label_value
    deploy_tag = args.deploy_tag

    for file in src_files:
        with open(file) as f:
            context = f.read()
            context = context.replace(r'job=\"tikv\"', r'job=\"{}\"'.format(engine_label_value))
            for e in replace_argv:
                src = 'tikv_{}'.format(e)
                tar = '{}{}'.format(prometheus_metric_name_prefix, e)
                context = context.replace(src, tar)
            for e in replace_argv2:
                src = e
                tar = '{}{}'.format(prometheus_metric_name_prefix, e)
                context = context.replace(src, tar)
            context = context.replace('tikv_', '{}tikv_'.format(prometheus_metric_name_prefix))
            if deploy_tag:
                context = context.replace('test-cluster', deploy_tag)
                context = context.replace(r'${DS_TEST-CLUSTER}', deploy_tag)
                context = context.replace('Test-Cluster', deploy_tag)
            with open('{}/{}'.format(output_dir, os.path.basename(f.name)), 'w') as o:
                o.write(context)
            o.close()
        f.close()


if __name__ == '__main__':
    main()

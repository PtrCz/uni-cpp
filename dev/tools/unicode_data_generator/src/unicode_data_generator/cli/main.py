from .parser import build_parser
from ..dispatcher.dispatcher import CommandDispatcher
from ..dispatcher import context

def main(argv=None):
    parser = build_parser()
    args = parser.parse_args(argv)

    global_context = context.GlobalContext(
        cache_dir=args.cache_dir if not getattr(args, 'no_cache', False) else None,
        output_dir=args.output_dir
    )

    dispatcher = CommandDispatcher(global_context)

    match args.command:
        case 'generate':
            generate_context = context.GenerateContext(
                unicode_version=args.unicode_version,
                target=args.target,
                dataset=getattr(args, 'dataset', None),
                use_precomputed_tuning=getattr(args, 'use_precomputed_tuning', None),
            )
            dispatcher.generate(generate_context)

        case 'analyze':
            analyze_context = context.AnalyzeContext(
                unicode_version=args.unicode_version,
                dataset=args.dataset,
            )
            dispatcher.analyze(analyze_context)

        case 'clean':
            clean_context = context.CleanContext(
                unicode_version=args.unicode_version,
                target=args.target,
            )
            dispatcher.clean(clean_context)

        case 'list':
            list_context = context.ListContext(
                target=args.target,
            )
            dispatcher.list(list_context)
#include <netcdfcpp.h>

#ifdef __linux__
#include <sys/sysinfo.h>
#endif

#include <cstdlib>
#include <map>
#include <vector>

namespace
{
    std::string infilename;
    std::string outfilename;
    std::vector<std::string> varstrings;

    std::vector<std::string> split(const std::string& str, const std::string& delim)
    {
        std::vector<std::string> result;

        std::string cpy(str);
        size_t pos = cpy.find_first_of(delim);
        while (pos != std::string::npos)
        {
            if (pos > 0)
            {
                result.push_back(cpy.substr(0, pos));
                cpy = cpy.substr(pos + 1);
                pos = cpy.find_first_of(delim);
            }
        }

        if (cpy.length() > 0)
        {
            result.push_back(cpy);
        }

        return result;
    }

    void printhelp()
    {
        std::cout << "ncextract [-v ...] infile outfile" << std::endl;
        std::cout << "  [-v var1[,...]]  Extract variable(s) <var1>,..." << std::endl;
        std::cout << "  infile           Name of netCDF input file" << std::endl;
        std::cout << "  outfile          Name of netCDF output file" << std::endl;
    }

    bool cmdline(int argc, char** argv)
    {
        if (argc <= 1)
        {
            return false;
        }

        opterr = 0;
        int c;
        while ((c = getopt(argc, argv, "hv:")) != -1)
        {
            switch (c)
            {
            case 'h':
                return false;
            case 'v':
                varstrings = split(optarg, ",");
                if (varstrings.size() == 0)
                {
                    return false;
                }
                break;
            default:
                break;
            }
        }

        // parse file name
        if (optind < argc)
        {
            infilename = argv[optind];
        }
        ++optind;

        if (optind < argc)
        {
            outfilename = argv[optind];
        }

        if (infilename == "" || outfilename == "")
        {
            return false;
        }

        return true;
    }
}

int main(int argc, char** argv)
{
    if (!cmdline(argc, argv))
    {
        printhelp();
        return EXIT_FAILURE;
    }

    NcFile infile(infilename.c_str(), NcFile::ReadOnly);
    if (!infile.is_valid())
    {
        std::cerr << "Error: invalid input file -- '" << infilename << "'" << std::endl;
        infile.close();
        return EXIT_FAILURE;
    }

    NcFile outfile(outfilename.c_str(), NcFile::Replace);
    if (!outfile.is_valid())
    {
        std::cerr << "Error: cannot open output file -- '" << outfilename << "'" << std::endl;
        outfile.close();
        return EXIT_FAILURE;
    }

    if (varstrings.size() == 0)
    {
        std::cerr << "Warning: no variables specified" << std::endl;
    }

    std::vector<NcVar*> invars;
    for (std::vector<std::string>::const_iterator it = varstrings.begin();
         it != varstrings.end(); ++it)
    {
        NcVar* var = infile.get_var((*it).c_str());
        if (var == NULL)
        {
            std::cerr << "Error: " << *it << ": no such variable" << std::endl;
            infile.close();
            outfile.close();
            return EXIT_FAILURE;
        }
        invars.push_back(var);
    }

    // extract the distinct set of dims
    std::map<std::string, NcDim*> indims;
    for (std::vector<NcVar*>::const_iterator it = invars.begin();
         it != invars.end(); ++it)
    {
        NcVar* var = *it;
        for (int i = 0; i < var->num_dims(); ++i)
        {
            NcDim* dim = var->get_dim(i);
            indims[dim->name()] = dim;
        }
    }

    // add dims to outfile
    std::map<std::string, NcDim*> outdims;
    for (std::map<std::string, NcDim*>::const_iterator it = indims.begin();
         it != indims.end(); ++it)
    {
        NcDim* dim = (*it).second;
        NcDim* outdim = NULL;
        if (dim->is_unlimited())
        {
            outdim = outfile.add_dim(dim->name());
        }
        else
        {
            outdim = outfile.add_dim(dim->name(), dim->size());
        }

        if (outdim != NULL)
        {
            outdims[outdim->name()] = outdim;
        }
    }

    // create variables
    for (std::vector<NcVar*>::const_iterator it = invars.begin();
         it != invars.end(); ++it)
    {
        NcVar* var = *it;
        std::vector<const NcDim*> dims(var->num_dims());
        for (int i = 0; i < var->num_dims(); ++i)
        {
            dims[i] = outdims[var->get_dim(i)->name()];
        }
        NcVar* outvar = outfile.add_var(var->name(), var->type(), var->num_dims(), &dims[0]);

#ifdef __linux__
        struct sysinfo info;
        sysinfo(&info);
#endif

        std::vector<long> cur;
        std::vector<long> counts;
        long len = 1;
        for (int i = 0; i < var->num_dims(); ++i)
        {
            cur.push_back(0);
            const NcDim* dim = var->get_dim(i);
            counts.push_back(dim->size());
            len *= dim->size();
            if (len >= info.freeram)
            {
                // TODO: paging
                std::cerr << "Error: variable " << var->name() << " too large to completely fit into main memory" << std::endl;
                infile.close();
                outfile.close();
                return EXIT_FAILURE;
            }
        }

        var->set_cur(&cur[0]);
        outvar->set_cur(&cur[0]);
        switch (outvar->type())
        {
        case ncByte:
        {
            ncbyte* barr = new ncbyte[len];
            var->get(barr, &counts[0]);
            outvar->put(barr, &counts[0]);
            delete[] barr;
            break;
        }
        case ncChar:
        {
            char* carr = new char[len];
            var->get(carr, &counts[0]);
            outvar->put(carr, &counts[0]);
            delete[] carr;
            break;
        }
        case ncShort:
        {
            short* sarr = new short[len];
            var->get(sarr, &counts[0]);
            outvar->put(sarr, &counts[0]);
            delete[] sarr;
            break;
        }
        case ncInt:
        {
            long* larr = new long[len];
            var->get(larr, &counts[0]);
            outvar->put(larr, &counts[0]);
            delete[] larr;
            break;
        }
        case ncFloat:
        {
            float* farr = new float[len];
            var->get(farr, &counts[0]);
            outvar->put(farr, &counts[0]);
            delete[] farr;
            break;
        }
        case ncDouble:
        {
            double* darr = new double[len];
            var->get(darr, &counts[0]);
            outvar->put(darr, &counts[0]);
            delete[] darr;
            break;
        }
        default:
            break;
        }
    }

    infile.close();
    outfile.close();
    return 0;
}


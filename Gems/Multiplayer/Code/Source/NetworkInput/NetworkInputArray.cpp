/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Source/NetworkInput/NetworkInputArray.h>
#include <Multiplayer/INetworkEntityManager.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/Serialization/DeltaSerializer.h>

namespace Multiplayer
{
    NetworkInputArray::NetworkInputArray()
        : m_owner()
        , m_inputs()
    {
        ;
    }

    NetworkInputArray::NetworkInputArray(const ConstNetworkEntityHandle& entityHandle)
        : m_owner(entityHandle)
        , m_inputs()
    {
        NetBindComponent* netBindComponent = entityHandle.GetNetBindComponent();
        if (netBindComponent)
        {
            for (AZStd::size_t i = 0; i < m_inputs.size(); ++i)
            {
                m_inputs[i].m_networkInput.AttachNetBindComponent(netBindComponent);
            }
        }
    }

    NetworkInput& NetworkInputArray::operator[](uint32_t index)
    {
        return m_inputs[index].m_networkInput;
    }

    const NetworkInput& NetworkInputArray::operator[](uint32_t index) const
    {
        return m_inputs[index].m_networkInput;
    }

    void NetworkInputArray::SetPreviousInputId(ClientInputId previousInputId)
    {
        m_previousInputId = previousInputId;
    }

    ClientInputId NetworkInputArray::GetPreviousInputId() const
    {
        return m_previousInputId;
    }

    bool NetworkInputArray::Serialize(AzNetworking::ISerializer& serializer)
    {
        // Always serialize the full first element
        if (!m_inputs[0].m_networkInput.Serialize(serializer))
        {
            return false;
        }

        // For each subsequent element
        for (uint32_t i = 1; i < m_inputs.size(); ++i)
        {
            if (serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject)
            {
                AzNetworking::SerializerDelta deltaSerializer;
                // Read out the delta
                if (!deltaSerializer.Serialize(serializer))
                {
                    return false;
                }
                // Start with previous value
                m_inputs[i].m_networkInput = m_inputs[i - 1].m_networkInput;
                // Then apply delta
                AzNetworking::DeltaSerializerApply applySerializer(deltaSerializer);
                if (!applySerializer.ApplyDelta(m_inputs[i].m_networkInput))
                {
                    return false;
                }
            }
            else
            {
                AzNetworking::SerializerDelta deltaSerializer;
                // Create the delta
                AzNetworking::DeltaSerializerCreate createSerializer(deltaSerializer);
                if (!createSerializer.CreateDelta(m_inputs[i - 1].m_networkInput, m_inputs[i].m_networkInput))
                {
                    return false;
                }
                // Then write out the delta
                if (!deltaSerializer.Serialize(serializer))
                {
                    return false;
                }
            }
        }
        serializer.Serialize(m_previousInputId, "PreviousInputId");
        return true;
    }
}